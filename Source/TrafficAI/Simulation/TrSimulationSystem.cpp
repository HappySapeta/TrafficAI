// Copyright Anupam Sahu. All Rights Reserved.

#include "TrSimulationSystem.h"
#include "TrSimulationData.h"
#include "RpSpatialGraphComponent.h"

//#define USE_ANGULAR_VELOCITY_BASED_STEERING

static bool GTrSimDebug = true;

FAutoConsoleCommand CComRenderDebug
(
	TEXT("trafficai.Debug"),
	TEXT("Draw traffic simulation debug information"),
	FConsoleCommandDelegate::CreateLambda
	(
		[]()
		{
			GTrSimDebug = !GTrSimDebug;
		}
	)
);

void UTrSimulationSystem::Initialize
(
	const UTrSimulationConfiguration* SimData,
	const UTrSpatialGraphComponent* GraphComponent,
	const TArray<FTrVehicleRepresentation>& TrafficEntities,
	const TArray<FTrVehiclePathTransform>& TrafficVehicleStarts
)
{
	NumEntities = TrafficEntities.Num();

	check(SimData)
	VehicleConfig = SimData->VehicleConfig;
	PathFollowingConfig = SimData->PathFollowingConfig;

	PathTransforms = TrafficVehicleStarts;
	check(PathTransforms.Num() > 0);

	Nodes = GraphComponent->GetNodes();
	IntersectionManager.Initialize(GraphComponent->GetIntersections());
	
	check(Nodes.Num() > 0);

	Positions = MakeShared<TArray<FVector>>();
		
	for (int Index = 0; Index < NumEntities; ++Index)
	{
		const AActor* EntityActor = TrafficEntities[Index].Dummy;

		Positions->Push(EntityActor->GetActorLocation());
		Velocities.Push(EntityActor->GetVelocity());
		Headings.Push(EntityActor->GetActorForwardVector());
		States.Push(ETrState::None);
		LeadingVehicleIndices.Push(-1);
		DebugColors.Push(FColor::MakeRandomColor());

		FVector NearestProjectionPoint;
		FindNearestPath(Index, NearestProjectionPoint);
		Goals.Push(NearestProjectionPoint);
	}

	GetWorld()->GetTimerManager().SetTimer
	(
		IntersectionTimerHandle,
		FTimerDelegate::CreateRaw(&IntersectionManager, &FTrIntersectionManager::Update),
		5.0f, // Interval
		true  // Loop
	);
	
	ImplicitGrid.Initialize(FFloatRange(-7000.0f, 7000.0f), 20, Positions);
	DrawFirstDebug();
}

void UTrSimulationSystem::GetVehicleTransforms(TArray<FTransform>& OutTransforms, const FVector& PositionOffset)
{
	if(OutTransforms.Num() < NumEntities)
	{
		OutTransforms.Init(FTransform::Identity, NumEntities);
	}
	
	for(int Index = 0; Index < NumEntities; ++Index)
	{
		FTransform Transform
		{
			Headings[Index].ToOrientationQuat(),
			Positions->operator[](Index) + PositionOffset
		};

		OutTransforms[Index] = MoveTemp(Transform);
	}
}

void UTrSimulationSystem::TickSimulation(const float DeltaSeconds)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UTrSimulationSystem::TickSimulation)

	TickRate = DeltaSeconds;
	
	DrawDebug();
	ImplicitGrid.Update();
	SetGoals();
	HandleGoals();
	UpdateCollisionData();
	UpdateKinematics();
	UpdateOrientations();
}

void UTrSimulationSystem::SetGoals()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UTrSimulationSystem::SetGoals)

	TArray<FVector> ProjectionPoints;
	ProjectionPoints.Reserve(PathTransforms.Num());

	for (int Index = 0; Index < NumEntities; ++Index)
	{
		const FVector Future = Positions->operator[](Index) + Velocities[Index].GetSafeNormal() * PathFollowingConfig.LookAheadDistance;

		const FVector PathDirection = (PathTransforms[Index].Path.End - PathTransforms[Index].Path.Start).GetSafeNormal();
		const FVector PathLeft = PathDirection.RotateAngleAxis(-90.0f, FVector::UpVector);
		const FVector PathOffset = PathLeft * PathFollowingConfig.PathFollowOffset;

		FTrPath OffsetPath = PathTransforms[Index].Path;
		OffsetPath.Start += PathOffset;
		OffsetPath.End += PathOffset;

		const FVector FutureOnPath = ProjectPointOnPath(Future, OffsetPath);
		const FVector PositionOnPath = ProjectPointOnPath(Positions->operator[](Index), OffsetPath);

		const float Distance = FVector::Distance(Positions->operator[](Index), PositionOnPath);
		if (Distance < PathFollowingConfig.PathFollowThreshold)
		{
			Goals[Index] = OffsetPath.End;
			States[Index] = ETrState::PathFollowing;
		}
		else
		{
			Goals[Index] = FutureOnPath;
			States[Index] = ETrState::PathInserting;
		}
	}
}

void UTrSimulationSystem::HandleGoals()
{
	for (int Index = 0; Index < NumEntities; ++Index)
	{
		ETrState CurrentState = States[Index];

		const float Distance = FVector::Distance(Goals[Index], Positions->operator[](Index));
		if (Distance <= PathFollowingConfig.GoalUpdateDistance && CurrentState == ETrState::PathFollowing)
		{
			UpdatePath(Index);
		}
	}
}

void UTrSimulationSystem::UpdateKinematics()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UTrSimulationSystem::UpdateKinematics)
	
	for (int Index = 0; Index < NumEntities; ++Index)
	{
		int LeadingVehicleIndex = LeadingVehicleIndices[Index];

		FVector& CurrentPosition = Positions->operator[](Index);
		const float CurrentSpeed = Velocities[Index].Size();
		float RelativeSpeed = CurrentSpeed;
		float CurrentGap = FVector::Distance(Goals[Index], CurrentPosition);
		float MinimumGap = 0.0f;

		if(LeadingVehicleIndex != -1)
		{
			const float DistanceToOther = FVector::Distance(CurrentPosition, Positions->operator[](LeadingVehicleIndex));
			if(DistanceToOther < CurrentGap)
			{
				MinimumGap = VehicleConfig.MinimumGap;
				CurrentGap = DistanceToOther;
				RelativeSpeed = ScalarProjection(Velocities[Index] - Velocities[LeadingVehicleIndex], Headings[Index]);
			}
		}
		
		const float FreeRoadTerm = VehicleConfig.MaximumAcceleration * (1 - FMath::Pow(CurrentSpeed / VehicleConfig.DesiredSpeed, VehicleConfig.AccelerationExponent));

		const float DecelerationTerm = (CurrentSpeed * RelativeSpeed) / (2 * FMath::Sqrt(VehicleConfig.MaximumAcceleration * VehicleConfig.ComfortableBrakingDeceleration));
		const float GapTerm = (MinimumGap + VehicleConfig.DesiredTimeHeadWay * CurrentSpeed + DecelerationTerm) / CurrentGap;
		const float InteractionTerm = -VehicleConfig.MaximumAcceleration * FMath::Square(GapTerm);

		float Acceleration = FreeRoadTerm + InteractionTerm;
		Acceleration = FMath::Clamp(Acceleration, -VehicleConfig.ComfortableBrakingDeceleration * 2.0f, VehicleConfig.MaximumAcceleration);

		FVector& CurrentHeading = Headings[Index];
		FVector& CurrentVelocity = Velocities[Index];

		CurrentVelocity += CurrentHeading * Acceleration * TickRate; // v = u + a * t
		CurrentPosition += CurrentVelocity * TickRate; // x1 = x0 + v * t
	}
}

void UTrSimulationSystem::UpdateOrientations()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UTrSimulationSystem::UpdateOrientations)
	
	for (int Index = 0; Index < NumEntities; ++Index)
	{
		FVector& CurrentHeading = Headings[Index];
		FVector& CurrentPosition = Positions->operator[](Index);
		FVector& CurrentVelocity = Velocities[Index];

		const FVector GoalDirection = (Goals[Index] - Positions->operator[](Index)).GetSafeNormal();

		FVector RearWheelPosition = CurrentPosition - CurrentHeading * VehicleConfig.WheelBaseLength * 0.5f;
		FVector FrontWheelPosition = CurrentPosition + CurrentHeading * VehicleConfig.WheelBaseLength * 0.5f;

		const FVector TargetHeading = (GoalDirection - CurrentHeading * 0.9f).GetSafeNormal();
		const float TargetSteerAngle = FMath::Atan2
		(
			CurrentHeading.X * TargetHeading.Y - CurrentHeading.Y * TargetHeading.X,
			CurrentHeading.X * TargetHeading.X + CurrentHeading.Y * TargetHeading.Y
		);

#ifndef USE_ANGULAR_VELOCITY_BASED_STEERING
		float SteerAngle = FMath::Clamp(TargetSteerAngle * VehicleConfig.SteeringSpeed, -VehicleConfig.MaxSteeringAngle, VehicleConfig.MaxSteeringAngle);

		RearWheelPosition += CurrentVelocity.Length() * CurrentHeading * TickRate;
		FrontWheelPosition += CurrentVelocity.Length() * CurrentHeading.RotateAngleAxis(FMath::RadiansToDegrees(SteerAngle), FVector::UpVector) * TickRate;

		CurrentHeading = (FrontWheelPosition - RearWheelPosition).GetSafeNormal();
		CurrentPosition = (FrontWheelPosition + RearWheelPosition) * 0.5f;
		CurrentVelocity = CurrentHeading * CurrentVelocity.Length();
#else 
		float SteerAngle = FMath::Clamp(TargetSteerAngle * VehicleConfig.SteeringSpeed, -VehicleConfig.MaxSteeringAngle, VehicleConfig.MaxSteeringAngle);

		const float TurningRadius = VehicleConfig.WheelBaseLength / FMath::Abs(FMath::Sin(SteerAngle));
		const float AngularSpeed = CurrentVelocity.Length() * FMath::Sign(SteerAngle) / TurningRadius;

		CurrentHeading = CurrentHeading.RotateAngleAxis(AngularSpeed, FVector::UpVector);
		CurrentVelocity = CurrentVelocity.Length() * CurrentHeading;
		CurrentPosition += CurrentVelocity * TickRate;
#endif
		
	}
}

void UTrSimulationSystem::UpdatePath(const uint32 Index)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UTrSimulationSystem::UpdatePath)
	
	FTrPath& CurrentPath = PathTransforms[Index].Path;
	const TArray<uint32>& Connections = Nodes[CurrentPath.EndNodeIndex].GetConnections();

	uint32 NewStartNodeIndex = CurrentPath.EndNodeIndex;
	uint32 NewEndNodeIndex = Connections[0];

	if(Connections.Num() == 2)
	{
		NewEndNodeIndex = Connections[0] == CurrentPath.StartNodeIndex ? Connections[1] : Connections[0];	
	}
	else
	{
		const FVector CurrentPathDirection = CurrentPath.Direction();

		TArray<uint32> EligibleConnections;
		for(uint32 Connection : Connections)
		{
			const FVector TargetPathDirection = (Nodes[Connection].GetLocation() - Nodes[NewStartNodeIndex].GetLocation()).GetSafeNormal();
			const float Angle = FMath::Acos(CurrentPathDirection.Dot(TargetPathDirection));

			if(Angle < PI / 2)
			{
				EligibleConnections.Push(Connection);
			}
		}

		const uint32 NumEligibleConnections = EligibleConnections.Num();
		if(NumEligibleConnections > 0)
		{
			if(NumEligibleConnections > 1 && IntersectionManager.IsNodeBlocked(NewStartNodeIndex))
			{
				return;
			}

			NewEndNodeIndex = EligibleConnections[FMath::RandRange(0, EligibleConnections.Num() - 1)];
		}
	}
	
	CurrentPath.Start = Nodes[NewStartNodeIndex].GetLocation();
	CurrentPath.End = Nodes[NewEndNodeIndex].GetLocation();
	CurrentPath.StartNodeIndex = NewStartNodeIndex;
	CurrentPath.EndNodeIndex = NewEndNodeIndex;
}

FVector UTrSimulationSystem::ProjectPointOnPath(const FVector& Point, const FTrPath& Path) const
{
	const FVector PathStart = Path.Start;
	const FVector PathEnd = Path.End;
	const FVector PathVector = PathEnd - PathStart;

	const FVector Temp = Point - PathStart;
	FVector Projection = (Temp.Dot(PathVector) / PathVector.Size()) * PathVector.GetSafeNormal() + PathStart;

	const float Alpha = FMath::Clamp((Projection - PathStart).Length() / PathVector.Length(), 0.0f, 1.0f);
	Projection = PathStart * (1 - Alpha) + PathEnd * Alpha;

	return Projection;
}

int UTrSimulationSystem::FindNearestPath(int EntityIndex, FVector& NearestProjection) const
{
	NearestProjection = Positions->operator[](EntityIndex);
	int NearestPathIndex = 0;
	float SmallestDistance = TNumericLimits<float>::Max();

	for (int PathIndex = 0; PathIndex < PathTransforms.Num(); ++PathIndex)
	{
		const FVector Future = Positions->operator[](EntityIndex) + Velocities[EntityIndex].GetSafeNormal() * PathFollowingConfig.LookAheadDistance;
		const FVector ProjectionPoint = ProjectPointOnPath(Future, PathTransforms[PathIndex].Path);
		const float Distance = FVector::Distance(ProjectionPoint, Positions->operator[](EntityIndex));

		if (Distance < SmallestDistance)
		{
			NearestProjection = ProjectionPoint;
			SmallestDistance = Distance;
			NearestPathIndex = PathIndex;
		}
	}
	return NearestPathIndex;
}

void UTrSimulationSystem::DrawDebug()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UTrSimulationSystem::DrawDebug)
	
	if (!GTrSimDebug)
	{
		return;
	}

	const float DebugLifeTime = TickRate * 2.0f;
	
	const UWorld* World = GetWorld();
	for (int Index = 0; Index < NumEntities; ++Index)
	{
		DrawDebugBox(World, Positions->operator[](Index), VehicleConfig.Dimensions, Headings[Index].ToOrientationQuat(), DebugColors[Index], false, DebugLifeTime);
		DrawDebugDirectionalArrow(World, Positions->operator[](Index), Positions->operator[](Index) + Headings[Index] * VehicleConfig.Dimensions.X * 1.5f, 1000.0f, FColor::Red, false, DebugLifeTime);
		DrawDebugPoint(World, Goals[Index], 2.0f, DebugColors[Index], false, DebugLifeTime);
		DrawDebugLine(World, Positions->operator[](Index), Goals[Index], DebugColors[Index], false, DebugLifeTime);
	}

	GEngine->AddOnScreenDebugMessage(-1, DebugLifeTime, FColor::Green, FString::Printf(TEXT("Speed : %.2f km/h"), Velocities[0].Length() * 0.036));
}

void UTrSimulationSystem::DrawFirstDebug()
{
	const UWorld* World = GetWorld();
	for (const FRpSpatialGraphNode& Node : Nodes)
	{
		const TArray<uint32>& Connections = Node.GetConnections();
		for (const uint32 ConnectedNodeIndex : Connections)
		{
			DrawDebugLine(World, Node.GetLocation(), Nodes[ConnectedNodeIndex].GetLocation(), FColor::White, true, -1);
		}
	}

	GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::White, FString::Printf(TEXT("Simulating %d vehicles"), NumEntities));
}

void UTrSimulationSystem::UpdateCollisionData()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UTrSimulationSystem::UpdateCollisionData)
	
	FRpSearchResults Results;
	const float Bound = VehicleConfig.Dimensions.Y * 2.0f; 
	
	for(int Index = 0; Index < NumEntities; ++Index)
	{
		Results.Reset();
		const FVector& CurrentPosition = Positions->operator[](Index);
		const FVector EndPosition = CurrentPosition + Headings[Index] * 2000.0f;
		ImplicitGrid.LineSearch(CurrentPosition + Headings[Index] * 250.0f, EndPosition, Results, GetWorld());

		LeadingVehicleIndices[Index] = -1;
		float ClosestDistance = TNumericLimits<float>().Max();
		FTransform CurrentTransform(Headings[Index].ToOrientationRotator(), CurrentPosition);
		uint8 Count = Results.Num();
		for(auto Itr = Results.Array.begin(); Count > 0; --Count, ++Itr)
		{
			const FVector& OtherPosition = Positions->operator[](*Itr);
			const FVector OtherLocalVector = CurrentTransform.InverseTransformPosition(OtherPosition);
			if(OtherLocalVector.Y >= -Bound && OtherLocalVector.Y <= Bound)
			{
				const float Distance = OtherLocalVector.X;
				if(Distance > 0.0f && Distance < ClosestDistance)
				{
					ClosestDistance = Distance;
					LeadingVehicleIndices[Index] = *Itr;
				}
			}
		}
	}
}
