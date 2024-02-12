// Copyright Anupam Sahu. All Rights Reserved.

#include "TrSimulationSystem.h"
#include "TrSimulationData.h"
#include "RpSpatialGraphComponent.h"

//#define USE_ANGULAR_VEL_STEERING

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
	const URpSpatialGraphComponent* GraphComponent,
	TWeakPtr<TArray<FTrVehicleRepresentation>> TrafficEntities,
	const TArray<FTrVehiclePathTransform>& TrafficVehicleStarts
)
{
	if (TrafficEntities.IsValid())
	{
		NumEntities = TrafficEntities.Pin()->Num();

		check(SimData)
		VehicleConfig = SimData->VehicleConfig;
		PathFollowingConfig = SimData->PathFollowingConfig;
		TickRate = SimData->TickRate;
		JunctionUpdateRate = SimData->JunctionUpdateRate;
		
		PathTransforms = TrafficVehicleStarts;
		check(PathTransforms.Num() > 0);

		Nodes = *GraphComponent->GetNodes();
		check(Nodes.Num() > 0);

		InitializeJunctions();

		for (int Index = 0; Index < NumEntities; ++Index)
		{
			const AActor* EntityActor = (*TrafficEntities.Pin())[Index].Dummy;

			Positions.Push(EntityActor->GetActorLocation());
			Velocities.Push(EntityActor->GetVelocity());
			Headings.Push(EntityActor->GetActorForwardVector());
			States.Push(ETrState::None);
			LeadingVehicleIndices.Push(-1);
			DebugColors.Push(FColor::MakeRandomColor());
			FTrPath& Path = PathTransforms[Index].Path;
			Path.Start += Path.Direction() * PathFollowingConfig.PathTrim;

			const float EndTrim = Junctions.Contains(Path.EndNodeIndex) ? PathFollowingConfig.JunctionExtents : PathFollowingConfig.PathTrim;
			Path.End -= Path.Direction() * EndTrim;
			

			FVector NearestProjectionPoint;
			Goals.Push(NearestProjectionPoint);
		}
	}

	DrawInitialDebug();
}

void UTrSimulationSystem::StartSimulation()
{
	const UWorld* World = GetWorld();

	FTimerDelegate SimTimerDelegate;
	SimTimerDelegate.BindUObject(this, &UTrSimulationSystem::TickSimulation);
	World->GetTimerManager().SetTimer(SimTimerHandle, SimTimerDelegate, TickRate, true);

	UpdateJunctions();
	FTimerDelegate JunctionTimerDelegate;
	JunctionTimerDelegate.BindUObject(this, &UTrSimulationSystem::UpdateJunctions);
	World->GetTimerManager().SetTimer(JunctionTimerHandle, JunctionTimerDelegate, JunctionUpdateRate, true);
}

void UTrSimulationSystem::StopSimulation()
{
	GetWorld()->GetTimerManager().ClearTimer(SimTimerHandle);
}

void UTrSimulationSystem::TickSimulation()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UTrSimulationSystem::TickSimulation)

	DebugVisualization();

	PathFollow();
	HandleGoal();
	SetAcceleration();
	UpdateVehicleSteer();
}

void UTrSimulationSystem::PathFollow()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UTrSimulationSystem::PathInsertion)

	TArray<FVector> ProjectionPoints;
	ProjectionPoints.Reserve(PathTransforms.Num());

	for (int Index = 0; Index < NumEntities; ++Index)
	{
		const FVector Future = Positions[Index] + Velocities[Index].GetSafeNormal() * PathFollowingConfig.LookAheadDistance;

		const FVector PathDirection = (PathTransforms[Index].Path.End - PathTransforms[Index].Path.Start).GetSafeNormal();
		const FVector PathLeft = PathDirection.RotateAngleAxis(-90.0f, FVector::UpVector);
		const FVector PathOffset = PathLeft * PathFollowingConfig.PathFollowOffset;

		FTrPath OffsetPath = PathTransforms[Index].Path;
		OffsetPath.Start += PathOffset;
		OffsetPath.End += PathOffset;

		const FVector FutureOnPath = ProjectPointOnPath(Future, OffsetPath);
		const FVector PositionOnPath = ProjectPointOnPath(Positions[Index], OffsetPath);

		const float Distance = FVector::Distance(Positions[Index], PositionOnPath);
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

void UTrSimulationSystem::UpdatePath(const uint32 Index)
{
	if (ShouldWaitAtJunction(Index))
	{
		return;
	}

	FTrPath& CurrentPath = PathTransforms[Index].Path;
	if(Nodes[CurrentPath.EndNodeIndex].GetConnections().Num() == 1)
	{
		return;
	}
	
	const TArray<uint32>& Connections = Nodes[CurrentPath.EndNodeIndex].GetConnections();

	uint32 NewStartNodeIndex = CurrentPath.EndNodeIndex;
	uint32 NewEndNodeIndex;
	do
	{
		NewEndNodeIndex = Connections[FMath::RandRange(0, Connections.Num() - 1)];
	}
	while (NewEndNodeIndex == CurrentPath.StartNodeIndex);

	CurrentPath.Start = Nodes[NewStartNodeIndex].GetLocation();
	CurrentPath.End = Nodes[NewEndNodeIndex].GetLocation();
	CurrentPath.StartNodeIndex = NewStartNodeIndex;
	CurrentPath.EndNodeIndex = NewEndNodeIndex;

	const FVector Direction = CurrentPath.Direction();

	const float StartTrim = PathFollowingConfig.PathTrim;
	const float EndTrim = Nodes[CurrentPath.EndNodeIndex].GetConnections().Num() > 2 ? PathFollowingConfig.JunctionExtents : PathFollowingConfig.PathTrim;

	CurrentPath.Start += Direction * StartTrim;
	CurrentPath.End -= Direction * EndTrim;
}

bool UTrSimulationSystem::ShouldWaitAtJunction(const uint32 Index)
{
	const FTrPath& CurrentPath = PathTransforms[Index].Path;
	const uint32 JunctionNodeIndex = CurrentPath.EndNodeIndex;

	if (Junctions.Contains(JunctionNodeIndex))
	{
		return Junctions[JunctionNodeIndex] != CurrentPath.StartNodeIndex;
	}

	return false;
}

void UTrSimulationSystem::HandleGoal()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UTrSimulationSystem::HandleGoal)

	for (int Index = 0; Index < NumEntities; ++Index)
	{
		ETrState CurrentState = States[Index];

		if (FVector::Distance(Goals[Index], Positions[Index]) <= PathFollowingConfig.GoalUpdateDistance)
		{
			switch (CurrentState)
			{
			case ETrState::PathFollowing:
				{
					UpdatePath(Index);
					break;
				}
			default:
				{
					// Do nothing
				}
			}
		}
	}
}

void UTrSimulationSystem::SetAcceleration()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UTrSimulationSystem::SetAcceleration)
	
	UpdateLeadingVehicles();
	for (int Index = 0; Index < NumEntities; ++Index)
	{
		int LeadingVehicleIndex = LeadingVehicleIndices[Index];

		FVector& CurrentPosition = Positions[Index];
		const float CurrentSpeed = Velocities[Index].Size();
		float RelativeSpeed = CurrentSpeed;
		float CurrentGap = FVector::Distance(Goals[Index], CurrentPosition);
		float MinimumGap = 0.0f;

		if(LeadingVehicleIndex != -1)
		{
			const float DistanceToOther = FVector::Distance(CurrentPosition, Positions[LeadingVehicleIndex]);
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
		Acceleration = FMath::Clamp(Acceleration, -10000.0f, VehicleConfig.MaximumAcceleration);

		FVector& CurrentHeading = Headings[Index];
		FVector& CurrentVelocity = Velocities[Index];

		CurrentVelocity += CurrentHeading * Acceleration * TickRate; // v = u + a * t
		CurrentPosition += CurrentVelocity * TickRate; // x1 = x0 + v * t
	}
}

#ifndef USE_ANGULAR_VEL_STEERING
void UTrSimulationSystem::UpdateVehicleSteer()
{
	for (int Index = 0; Index < NumEntities; ++Index)
	{
		FVector& CurrentHeading = Headings[Index];
		FVector& CurrentPosition = Positions[Index];
		FVector& CurrentVelocity = Velocities[Index];

		const FVector GoalDirection = (Goals[Index] - Positions[Index]).GetSafeNormal();

		FVector RearWheelPosition = CurrentPosition - CurrentHeading * VehicleConfig.WheelBaseLength * 0.5f;
		FVector FrontWheelPosition = CurrentPosition + CurrentHeading * VehicleConfig.WheelBaseLength * 0.5f;

		const FVector TargetHeading = (GoalDirection - CurrentHeading * 0.9f).GetSafeNormal();
		const float TargetSteerAngle = FMath::Atan2
		(
			CurrentHeading.X * TargetHeading.Y - CurrentHeading.Y * TargetHeading.X,
			CurrentHeading.X * TargetHeading.X + CurrentHeading.Y * TargetHeading.Y
		);

		float SteerAngle = FMath::Clamp(TargetSteerAngle * VehicleConfig.SteeringSpeed, -VehicleConfig.MaxSteeringAngle, VehicleConfig.MaxSteeringAngle);

		RearWheelPosition += CurrentVelocity.Length() * CurrentHeading * TickRate;
		FrontWheelPosition += CurrentVelocity.Length() * CurrentHeading.RotateAngleAxis(FMath::RadiansToDegrees(SteerAngle), FVector::UpVector) * TickRate;

		CurrentHeading = (FrontWheelPosition - RearWheelPosition).GetSafeNormal();
		CurrentPosition = (FrontWheelPosition + RearWheelPosition) * 0.5f;
		CurrentVelocity = CurrentHeading * CurrentVelocity.Length();
	}
	
}
#endif

#ifdef USE_ANGULAR_VEL_STEERING
void UTrSimulationSystem::UpdateVehicleSteer()
{
	for (int Index = 0; Index < NumEntities; ++Index)
	{
		FVector& CurrentHeading = Headings[Index];
		FVector& CurrentPosition = Positions[Index];
		FVector& CurrentVelocity = Velocities[Index];

		const FVector GoalDirection = (Goals[Index] - Positions[Index]).GetSafeNormal();

		const FVector TargetHeading = (GoalDirection - CurrentHeading * 0.9f).GetSafeNormal();
		const float TargetSteerAngle = FMath::Atan2
		(
			CurrentHeading.X * TargetHeading.Y - CurrentHeading.Y * TargetHeading.X,
			CurrentHeading.X * TargetHeading.X + CurrentHeading.Y * TargetHeading.Y
		);

		float SteerAngle = FMath::Clamp(TargetSteerAngle * VehicleConfig.SteeringSpeed, -VehicleConfig.MaxSteeringAngle, VehicleConfig.MaxSteeringAngle);

		const float TurningRadius = VehicleConfig.WheelBaseLength / FMath::Abs(FMath::Sin(SteerAngle));
		const float AngularSpeed = CurrentVelocity.Length() * FMath::Sign(SteerAngle) / TurningRadius;

		CurrentHeading = CurrentHeading.RotateAngleAxis(AngularSpeed, FVector::UpVector);
		CurrentVelocity = CurrentVelocity.Length() * CurrentHeading;
		CurrentPosition += CurrentVelocity * TickRate;
	}
}
#endif

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

int UTrSimulationSystem::FindNearestPath(int EntityIndex, FVector& NearestProjection)
{
	NearestProjection = Positions[EntityIndex];
	int NearestPathIndex = 0;
	float SmallestDistance = TNumericLimits<float>::Max();

	for (int PathIndex = 0; PathIndex < PathTransforms.Num(); ++PathIndex)
	{
		const FVector Future = Positions[EntityIndex] + Velocities[EntityIndex].GetSafeNormal() * PathFollowingConfig.LookAheadDistance;
		const FVector ProjectionPoint = ProjectPointOnPath(Future, PathTransforms[PathIndex].Path);
		const float Distance = FVector::Distance(ProjectionPoint, Positions[EntityIndex]);

		if (Distance < SmallestDistance)
		{
			NearestProjection = ProjectionPoint;
			SmallestDistance = Distance;
			NearestPathIndex = PathIndex;
		}
	}
	return NearestPathIndex;
}

void UTrSimulationSystem::DebugVisualization()
{
	if (!GTrSimDebug)
	{
		return;
	}

	const UWorld* World = GetWorld();
	for (int Index = 0; Index < NumEntities; ++Index)
	{
		DrawDebugBox(World, Positions[Index], VehicleConfig.Dimensions, Headings[Index].ToOrientationQuat(), DebugColors[Index], false, TickRate);
		DrawDebugDirectionalArrow(World, Positions[Index], Positions[Index] + Headings[Index] * VehicleConfig.Dimensions.X * 1.5f, 1000.0f, FColor::Red, false, TickRate);
		DrawDebugPoint(World, Goals[Index], 2.0f, DebugColors[Index], false, TickRate);
		DrawDebugLine(World, Positions[Index], Goals[Index], DebugColors[Index], false, TickRate);
	}

	GEngine->AddOnScreenDebugMessage(-1, GetWorld()->GetDeltaSeconds(), FColor::Green, FString::Printf(TEXT("Speed : %.2f km/h"), Velocities[0].Length() * 0.036));
}

void UTrSimulationSystem::DrawInitialDebug()
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

void UTrSimulationSystem::InitializeJunctions()
{
	for (uint32 NodeIndex = 0, NumNodes = Nodes.Num(); NodeIndex < NumNodes; ++NodeIndex)
	{
		const FRpSpatialGraphNode& Node = Nodes[NodeIndex];
		const TArray<uint32>& Connections = Node.GetConnections();
		if (Connections.Num() > 2)
		{
			Junctions.Add(NodeIndex, Connections[0]);
		}
	}
}

void UTrSimulationSystem::UpdateJunctions()
{
	for (auto& Junction : Junctions)
	{
		const TArray<uint32>& Connections = Nodes[Junction.Key].GetConnections();
		Junction.Value = Connections[FMath::RandRange(0, Connections.Num() - 1)];

		const FVector Direction = (Nodes[Junction.Value].GetLocation() - Nodes[Junction.Key].GetLocation()).GetSafeNormal();
		DrawDebugPoint(GetWorld(), Nodes[Junction.Key].GetLocation() + Direction * PathFollowingConfig.JunctionExtents, 3.0f, FColor::Green, false, JunctionUpdateRate);
	}
}

void UTrSimulationSystem::UpdateLeadingVehicles()
{
	const float Bound = VehicleConfig.Dimensions.Y; 
	for(int Index = 0; Index < NumEntities; ++Index)
	{
		LeadingVehicleIndices[Index] = -1;
		float ClosestDistance = TNumericLimits<float>().Max();
		FTransform CurrentTransform(Headings[Index].ToOrientationRotator(), Positions[Index]);
		for(int OtherIndex = 0; OtherIndex < NumEntities; ++OtherIndex)
		{
			if(OtherIndex == Index)
			{
				continue;
			}

			const FVector OtherLocalVector = CurrentTransform.InverseTransformPosition(Positions[OtherIndex]);

			if(OtherLocalVector.Y >= -Bound && OtherLocalVector.Y <= Bound)
			{
				const float Distance = OtherLocalVector.X;
				if((Distance > VehicleConfig.Dimensions.X / 2) && Distance < ClosestDistance)
				{
					ClosestDistance = Distance;
					LeadingVehicleIndices[Index] = OtherIndex;
				}
			}
		}

		const int LeadingVehicleIndex = LeadingVehicleIndices[Index];
		if(LeadingVehicleIndex != -1)
		{
			DrawDebugLine(GetWorld(), Positions[Index], Positions[LeadingVehicleIndex], DebugColors[LeadingVehicleIndex], false, TickRate);
		}
	}
}
