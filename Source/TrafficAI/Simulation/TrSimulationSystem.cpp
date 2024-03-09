// Copyright Anupam Sahu. All Rights Reserved.

#include "TrSimulationSystem.h"
#include "TrSimulationData.h"
#include "RpSpatialGraphComponent.h"

#define DEBUG_LIFETIME -1
constexpr float AMBER_DURATION = 5.0f; // This duration is used for the timer that switches the signal state from green to amber.
constexpr float DETECTION_RANGE_SCALE = 2.0f; // Values smaller than 2 would result in failure to detect other vehicles properly.

static bool GAIDebug = false;
static FAutoConsoleCommand CComToggleAIDebug
(
	TEXT("Traffic.AIDebug"),
	TEXT(""),
	FConsoleCommandDelegate::CreateLambda([]()
	{
		GAIDebug = !GAIDebug;		
	}),
	ECVF_Default
);

static bool GCollisionDebug = false;
static FAutoConsoleCommand CComToggleCollisionDebug
(
	TEXT("Traffic.CollisionDebug"),
	TEXT(""),
	FConsoleCommandDelegate::CreateLambda([]()
	{
		GCollisionDebug = !GCollisionDebug;		
	}),
	ECVF_Default
);

static bool GGridDebug = false;
static FAutoConsoleCommand CComToggleGridDebug
(
	TEXT("Traffic.GridDebug"),
	TEXT(""),
	FConsoleCommandDelegate::CreateLambda([]()
	{
		GGridDebug = !GGridDebug;		
	}),
	ECVF_Default
);

void UTrSimulationSystem::Initialize
(
	const UTrSimulationConfiguration* SimData,
	const UTrSpatialGraphComponent* GraphComponent,
	const TArray<FTransform>& InitialTransforms,
	const TArray<FTrVehiclePathTransform>& TrafficVehicleStarts
)
{
	NumEntities = InitialTransforms.Num();

	check(SimData)
	VehicleConfig = SimData->VehicleConfig;
	PathFollowingConfig = SimData->PathFollowingConfig;

	PathTransforms = TrafficVehicleStarts;
	check(PathTransforms.Num() > 0);

	Nodes = GraphComponent->GetNodes();
	
	check(Nodes.Num() > 0);
	for (int Index = 0; Index < NumEntities; ++Index)
	{
		Positions.Push(InitialTransforms[Index].GetLocation());
		Velocities.Push(FVector::Zero());
		Headings.Push(InitialTransforms[Index].GetRotation().GetForwardVector());
		LeadingVehicleIndices.Push(-1);
		PathFollowingStates.Push(false);
		FVector NearestProjectionPoint;
		FindNearestPath(Index, NearestProjectionPoint);
		Goals.Push(NearestProjectionPoint);
		
#if !UE_BUILD_SHIPPING
		DebugColors.Push(FColor::MakeRandomColor());
#endif
	}

	IntersectionManager.Initialize(GraphComponent->GetIntersections());

	const float SignalSwitchTime = SimData->PathFollowingConfig.SignalSwitchInterval;
	GetWorld()->GetTimerManager().SetTimer
	(
		IntersectionTimerHandle,
		FTimerDelegate::CreateRaw(&IntersectionManager, &FTrIntersectionManager::SwitchToGreen),
		SignalSwitchTime,
		true
	);

	GetWorld()->GetTimerManager().SetTimer
	(
		AmberTimerHandle,
		FTimerDelegate::CreateRaw(&IntersectionManager, &FTrIntersectionManager::SwitchToAmber),
		FMath::Max(1, SignalSwitchTime - AMBER_DURATION),
		true
	);
	
	ImplicitGrid.Initialize(FFloatRange(-SimData->GridConfiguration.Range, SimData->GridConfiguration.Range), SimData->GridConfiguration.Resolution);
	GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::White, FString::Printf(TEXT("Simulating %d vehicles"), NumEntities));
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
			Positions[Index] + PositionOffset
		};

		OutTransforms[Index] = Transform;
	}
}

void UTrSimulationSystem::TickSimulation(const float DeltaSeconds)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UTrSimulationSystem::TickSimulation)

	TickRate = DeltaSeconds;
	
	DrawDebug();
	ImplicitGrid.Update(Positions);
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
		const FVector Future = Positions[Index] + Velocities[Index].GetSafeNormal() * PathFollowingConfig.LookAheadDistance;

		const FVector PathDirection = (PathTransforms[Index].Path.End - PathTransforms[Index].Path.Start).GetSafeNormal();
		const FVector PathLeft = PathDirection.RotateAngleAxis(-90.0f, FVector::UpVector);
		const FVector PathOffset = PathLeft * PathFollowingConfig.PathFollowOffset;

		FTrPath OffsetPath = PathTransforms[Index].Path;
		OffsetPath.Start += PathOffset;
		OffsetPath.End += PathOffset;

		const FVector FutureOnPath = ProjectPointOnPathClamped(Future, OffsetPath);
		const FVector PositionOnPath = ProjectPointOnPathClamped(Positions[Index], OffsetPath);

		const float Distance = FVector::Distance(Positions[Index], PositionOnPath);
		if (Distance < PathFollowingConfig.PathFollowThreshold)
		{
			Goals[Index] = OffsetPath.End;
			PathFollowingStates[Index] = true;
		}
		else
		{
			Goals[Index] = FutureOnPath;
			PathFollowingStates[Index] = false;
		}
	}
}

void UTrSimulationSystem::HandleGoals()
{
	for (int Index = 0; Index < NumEntities; ++Index)
	{
		const float Distance = FVector::Distance(Goals[Index], Positions[Index]);
		if (Distance <= PathFollowingConfig.GoalUpdateDistance && PathFollowingStates[Index] == true)
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

FVector UTrSimulationSystem::ProjectPointOnPathClamped(const FVector& Point, const FTrPath& Path) const
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
	NearestProjection = Positions[EntityIndex];
	int NearestPathIndex = 0;
	float SmallestDistance = TNumericLimits<float>::Max();

	for (int PathIndex = 0; PathIndex < PathTransforms.Num(); ++PathIndex)
	{
		const FVector Future = Positions[EntityIndex] + Velocities[EntityIndex].GetSafeNormal() * PathFollowingConfig.LookAheadDistance;
		const FVector ProjectionPoint = ProjectPointOnPathClamped(Future, PathTransforms[PathIndex].Path);
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

void UTrSimulationSystem::UpdateCollisionData()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UTrSimulationSystem::UpdateCollisionData)
	
	FRpSearchResults Results;
	const float Bound = VehicleConfig.Dimensions.Y * DETECTION_RANGE_SCALE; 

	const UWorld* World = GetWorld();
	
	for(int Index = 0; Index < NumEntities; ++Index)
	{
		Results.Reset();
		const FVector& CurrentPosition = Positions[Index];
		const FVector EndPosition = CurrentPosition + Headings[Index] * VehicleConfig.CollisionSensorRange;
		ImplicitGrid.LineSearch(CurrentPosition, EndPosition, Results);

		LeadingVehicleIndices[Index] = -1;
		float ClosestDistance = TNumericLimits<float>().Max();
		FTransform CurrentTransform(Headings[Index].ToOrientationRotator(), CurrentPosition);
		uint8 Count = Results.Num();
		for(auto Itr = Results.Array.begin(); Count > 0; --Count, ++Itr)
		{
			const FVector& OtherPosition = Positions[*Itr];
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

		if(GCollisionDebug && LeadingVehicleIndices[Index] != -1)
		{
			const FVector& OtherPosition = Positions[LeadingVehicleIndices[Index]];
			DrawDebugLine(World, CurrentPosition, OtherPosition, FColor::Red, false, DEBUG_LIFETIME);
		}
	}
}

#if !UE_BUILD_SHIPPING
void UTrSimulationSystem::DrawDebug()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UTrSimulationSystem::DrawDebug)

	const UWorld* World = GetWorld();
	if(GGridDebug)
	{
		ImplicitGrid.DrawDebug(World, DEBUG_LIFETIME);
	}

	if(GAIDebug)
	{
		for (int Index = 0; Index < NumEntities; ++Index)
		{
			DrawGraph(World);
			DrawDebugBox(World, Positions[Index], VehicleConfig.Dimensions, Headings[Index].ToOrientationQuat(), DebugColors[Index], false, DEBUG_LIFETIME);
			DrawDebugDirectionalArrow(World, Positions[Index], Positions[Index] + Headings[Index] * VehicleConfig.Dimensions.X * 1.5f, 1000.0f, FColor::Red, false, DEBUG_LIFETIME);
			DrawDebugPoint(World, Goals[Index], 2.0f, DebugColors[Index], false, DEBUG_LIFETIME);
			DrawDebugLine(World, Positions[Index], Goals[Index], DebugColors[Index], false, DEBUG_LIFETIME);
		}
	}
}

void UTrSimulationSystem::DrawGraph(const UWorld* World)
{
	for (const FRpSpatialGraphNode& Node : Nodes)
	{
		const TArray<uint32>& Connections = Node.GetConnections();
		for (const uint32 ConnectedNodeIndex : Connections)
		{
			DrawDebugLine(World, Node.GetLocation(), Nodes[ConnectedNodeIndex].GetLocation(), FColor::White, false, DEBUG_LIFETIME);
		}
	}
}
#endif

void UTrSimulationSystem::BeginDestroy()
{
	if(const UWorld* World = GetWorld())
	{
		FTimerManager& TimerManager = World->GetTimerManager();
		TimerManager.ClearTimer(IntersectionTimerHandle);
		TimerManager.ClearTimer(AmberTimerHandle);
	}
	Super::BeginDestroy();
}
