// Copyright Anupam Sahu. All Rights Reserved.

#include "TrSimulationSystem.h"
#include "TrSimulationData.h"
#include "RpSpatialGraphComponent.h"

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
			Accelerations.Push(0.0f);
			SteerAngles.Push(0.0f);
			States.Push(ETrState::None);
			DebugColors.Push(FColor::MakeRandomColor());

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
	UpdateVehicle();
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
	const TArray<uint32>& Connections = Nodes[CurrentPath.EndNodeIndex].GetConnections().Array();

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

	for (int Index = 0; Index < NumEntities; ++Index)
	{
		float& Acceleration = Accelerations[Index];
		int LeadingVehicleIndex = -1;

		float DesiredSpeed = VehicleConfig.DesiredSpeed;
		const float GoalDistance = FVector::Distance(Goals[Index], Positions[Index]);

		const float CurrentSpeed = Velocities[Index].Size();
		const float RelativeSpeed = LeadingVehicleIndex != -1 ? Velocities[LeadingVehicleIndex].Size() : 0.0f;
		const float CurrentGap = LeadingVehicleIndex != -1 ? FVector::Distance(Positions[LeadingVehicleIndex], Positions[Index]) : GoalDistance;
		
		const float FreeRoadTerm = VehicleConfig.MaximumAcceleration * (1 - FMath::Pow(CurrentSpeed / DesiredSpeed, VehicleConfig.AccelerationExponent));

		const float DecelerationTerm = (CurrentSpeed * RelativeSpeed) / (2 * FMath::Sqrt(VehicleConfig.MaximumAcceleration * VehicleConfig.ComfortableBrakingDeceleration));
		const float GapTerm = (VehicleConfig.MinimumGap + VehicleConfig.DesiredTimeHeadWay * CurrentSpeed + DecelerationTerm) /CurrentGap;
		const float InteractionTerm = -VehicleConfig.MaximumAcceleration * FMath::Square(GapTerm);

		Acceleration = FreeRoadTerm + InteractionTerm;
		Acceleration = FMath::Clamp(Acceleration, -10000.0f, VehicleConfig.MaximumAcceleration);
	}
}

void UTrSimulationSystem::UpdateVehicle()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UTrSimulationSystem::UpdateVehicle)

	for (int Index = 0; Index < NumEntities; ++Index)
	{
		UpdateVehicleKinematics(Index);
		UpdateVehicleSteer(Index);
	}
}

void UTrSimulationSystem::UpdateVehicleKinematics(const int Index)
{
	FVector& CurrentHeading = Headings[Index];
	FVector& CurrentPosition = Positions[Index];
	FVector& CurrentVelocity = Velocities[Index];

	CurrentVelocity += CurrentHeading * Accelerations[Index] * TickRate; // v = u + a * t
	CurrentPosition += CurrentVelocity * TickRate; // x1 = x0 + v * t
}

void UTrSimulationSystem::UpdateVehicleSteer(const int Index)
{
	FVector& CurrentHeading = Headings[Index];
	FVector& CurrentPosition = Positions[Index];
	FVector& CurrentVelocity = Velocities[Index];
	float& CurrentSteerAngle = SteerAngles[Index];

	const FVector GoalDirection = (Goals[Index] - Positions[Index]).GetSafeNormal();

	FVector RearWheelPosition = CurrentPosition - CurrentHeading * VehicleConfig.WheelBaseLength * 0.5f;
	FVector FrontWheelPosition = CurrentPosition + CurrentHeading * VehicleConfig.WheelBaseLength * 0.5f;

	const FVector TargetHeading = (GoalDirection - CurrentHeading * 0.9f).GetSafeNormal();
	const float TargetSteerAngle = FMath::Atan2
	(
		CurrentHeading.X * TargetHeading.Y - CurrentHeading.Y * TargetHeading.X,
		CurrentHeading.X * TargetHeading.X + CurrentHeading.Y * TargetHeading.Y
	);

	const float Delta = FMath::Clamp(TargetSteerAngle - CurrentSteerAngle, -VehicleConfig.SteeringSpeed, VehicleConfig.SteeringSpeed);
	CurrentSteerAngle += Delta;
	CurrentSteerAngle = FMath::Clamp(CurrentSteerAngle, -VehicleConfig.MaxSteeringAngle, VehicleConfig.MaxSteeringAngle);

	RearWheelPosition += CurrentVelocity.Length() * CurrentHeading * TickRate;
	FrontWheelPosition += CurrentVelocity.Length() * CurrentHeading.RotateAngleAxis(
		FMath::RadiansToDegrees(CurrentSteerAngle), FVector::UpVector) * TickRate;

	CurrentHeading = (FrontWheelPosition - RearWheelPosition).GetSafeNormal();
	CurrentPosition = (FrontWheelPosition + RearWheelPosition) * 0.5f;
	CurrentVelocity = CurrentHeading * CurrentVelocity.Length();
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
	GEngine->AddOnScreenDebugMessage(-1, GetWorld()->GetDeltaSeconds(), FColor::White, FString::Printf(TEXT("Acceleration : %.2f"), Accelerations[0]));
}

void UTrSimulationSystem::DrawInitialDebug()
{
	const UWorld* World = GetWorld();
	for (const FRpSpatialGraphNode& Node : Nodes)
	{
		for (const uint32 ConnectedNodeIndex : Node.GetConnections())
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
		const TArray<uint32> Connections = Node.GetConnections().Array();
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
		const TArray<uint32> Connections = Nodes[Junction.Key].GetConnections().Array();
		Junction.Value = Connections[FMath::RandRange(0, Connections.Num() - 1)];

		const FVector Direction = (Nodes[Junction.Value].GetLocation() - Nodes[Junction.Key].GetLocation()).GetSafeNormal();
		DrawDebugPoint(GetWorld(), Nodes[Junction.Key].GetLocation() + Direction * PathFollowingConfig.JunctionExtents, 3.0f, FColor::Green, false, JunctionUpdateRate);
	}
}
