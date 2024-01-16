// Copyright Anupam Sahu. All Rights Reserved.

#include "TrSimulationSystem.h"
#include "RpSpatialGraphComponent.h"

// Vehicle Dimensions
const FVector VEHICLE_EXTENTS(465, 179, 143);
constexpr float WHEEL_BASE = 270;

// Speed limits
constexpr float MAX_SPEED = 1000.0f; // 1000 : 36 km/h TODO : replace with ModelData.DesiredSpeed.

// Timing
constexpr float FIXED_DELTA_TIME = 0.016f;
constexpr float LOOK_AHEAD_TIME = FIXED_DELTA_TIME * 100;

// Ranges
constexpr float PATH_RADIUS = 200.0f; // 300 : 3 m
constexpr float GOAL_RADIUS = 500.0f; // 500 : 5m

// Approach
const TRange<float> APPROACH_SPEED_RANGE(277.778f, MAX_SPEED);
constexpr float APPROACH_RADIUS = 2000.0f; // 1000 : 10 m

// Steering Configuration
constexpr float STEERING_SPEED = 0.1f;
constexpr float MAX_STEER_ANGLE = (UE_PI / 180.f) * 45.0f; // degree to radian conversion

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

void UTrSimulationSystem::Initialize(const URpSpatialGraphComponent* GraphComponent, TWeakPtr<TArray<FTrVehicleRepresentation>> TrafficEntities, const TArray<FTrVehiclePathTransform>& TrafficVehicleStarts)
{
	if(TrafficEntities.IsValid())
	{
		NumEntities = TrafficEntities.Pin()->Num();
		
		PathTransforms = TrafficVehicleStarts;
		check(PathTransforms.Num() > 0);

		Nodes = *GraphComponent->GetNodes();
		check(Nodes.Num() > 0);
		
		for(int Index = 0; Index < NumEntities; ++Index)
		{
			const AActor* EntityActor = (*TrafficEntities.Pin())[Index].Dummy;
			
			Positions.Push(EntityActor->GetActorLocation());
			Velocities.Push(EntityActor->GetVelocity());
			Headings.Push(EntityActor->GetActorForwardVector());
			Accelerations.Push(0.0f);
			SteerAngles.Push(0.0f);
			DebugColors.Push(FColor::MakeRandomColor());

			FVector NearestProjectionPoint;
			Goals.Push(NearestProjectionPoint);
		}
	}
	
	DrawInitialDebug();
}

void UTrSimulationSystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UTrSimulationSystem::StartSimulation()
{
	FTimerDelegate SimTimerDelegate;
	SimTimerDelegate.BindUObject(this, &UTrSimulationSystem::TickSimulation);
	GetWorld()->GetTimerManager().SetTimer(SimTimerHandle, SimTimerDelegate, FIXED_DELTA_TIME, true);
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
	
	for(int Index = 0; Index < NumEntities; ++Index)
	{
		const FVector Future = Positions[Index] + Velocities[Index] * LOOK_AHEAD_TIME;

		const FVector PathDirection = (PathTransforms[Index].Path.End - PathTransforms[Index].Path.Start).GetSafeNormal();
		const FVector PathLeft = PathDirection.RotateAngleAxis(-90.0f, FVector::UpVector);
		const FVector PathOffset = PathLeft * PATH_RADIUS;
		
		FTrPath OffsetPath = PathTransforms[Index].Path;
		OffsetPath.Start += PathOffset;
		OffsetPath.End += PathOffset;
		
		const FVector FutureOnPath = ProjectPointOnPath(Future, OffsetPath);
		const FVector PositionOnPath = ProjectPointOnPath(Positions[Index], OffsetPath);
		
		const float Distance = FVector::Distance(Positions[Index], PositionOnPath);
		if(Distance < PATH_RADIUS)
		{
			Goals[Index] = OffsetPath.End;
		}
		else if(Distance >= PATH_RADIUS)
		{
			Goals[Index] = FutureOnPath;
		}
	}
}

void UTrSimulationSystem::HandleGoal()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UTrSimulationSystem::HandleGoal)
	
	for(int Index = 0; Index < NumEntities; ++Index)
	{
		FTrPath& CurrentPath = PathTransforms[Index].Path;
		if(FVector::Distance(Goals[Index], Positions[Index]) <= GOAL_RADIUS && FVector::PointsAreNear(Goals[Index], CurrentPath.End, PATH_RADIUS * 1.01f))
		{
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
		}
	}
}

void UTrSimulationSystem::SetAcceleration()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UTrSimulationSystem::SetAcceleration)
	
	for(int Index = 0; Index < NumEntities; ++Index)
	{
		float& Acceleration = Accelerations[Index];
		int LeadingVehicleIndex = -1;

		float DesiredSpeed = MAX_SPEED;
		const float GoalDistance = FVector::Distance(Goals[Index], Positions[Index]);
		if(GoalDistance < APPROACH_RADIUS)
		{
			DesiredSpeed = FMath::Max((GoalDistance / APPROACH_RADIUS) * APPROACH_SPEED_RANGE.GetUpperBoundValue(), APPROACH_SPEED_RANGE.GetLowerBoundValue());
		}
		
		const float CurrentSpeed = Velocities[Index].Size();
		const float RelativeSpeed = LeadingVehicleIndex != -1 ? Velocities[LeadingVehicleIndex].Size() : 0.0f;
		const float CurrentGap = LeadingVehicleIndex != -1 ? FVector::Distance(Positions[LeadingVehicleIndex], Positions[Index]) : TNumericLimits<float>::Max();
		
		const float FreeRoadTerm = ModelData.MaximumAcceleration * (1 - FMath::Pow(CurrentSpeed / DesiredSpeed, ModelData.AccelerationExponent));

		const float DecelerationTerm = (CurrentSpeed * RelativeSpeed) / (2 * FMath::Sqrt(ModelData.MaximumAcceleration * ModelData.ComfortableBrakingDeceleration));
		const float GapTerm = (ModelData.MinimumGap + ModelData.DesiredTimeHeadWay * CurrentSpeed + DecelerationTerm) / CurrentGap;
		const float InteractionTerm = -ModelData.MaximumAcceleration * FMath::Square(GapTerm);

		Acceleration = FreeRoadTerm + InteractionTerm;
	}
}

void UTrSimulationSystem::UpdateVehicle()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UTrSimulationSystem::UpdateVehicle)
	
	for(int Index = 0; Index < NumEntities; ++Index)
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

	CurrentVelocity += CurrentHeading * Accelerations[Index] * FIXED_DELTA_TIME; // v = u + a * t

	if(CurrentVelocity.Length() > MAX_SPEED)
	{
		CurrentVelocity = CurrentVelocity.GetSafeNormal() * MAX_SPEED;
	}
		
	CurrentPosition += CurrentVelocity * FIXED_DELTA_TIME; // x1 = x0 + v * t
}

void UTrSimulationSystem::UpdateVehicleSteer(const int Index)
{
	FVector& CurrentHeading = Headings[Index];
	FVector& CurrentPosition = Positions[Index];
	FVector& CurrentVelocity = Velocities[Index];
	float& CurrentSteerAngle = SteerAngles[Index];
	
	FVector RearWheelPosition = CurrentPosition - CurrentHeading * WHEEL_BASE * 0.5f;
	FVector FrontWheelPosition = CurrentPosition + CurrentHeading * WHEEL_BASE * 0.5f;

	const FVector TargetHeading = (Goals[Index] - Positions[Index]).GetSafeNormal();
	const float TargetSteerAngle = FMath::Atan2
					(
						CurrentHeading.X * TargetHeading.Y - CurrentHeading.Y * TargetHeading.X,
						CurrentHeading.X * TargetHeading.X + CurrentHeading.Y * TargetHeading.Y
					);

	const float Delta = TargetSteerAngle - CurrentSteerAngle;
	CurrentSteerAngle += Delta * STEERING_SPEED;
	CurrentSteerAngle = FMath::Clamp(CurrentSteerAngle, -MAX_STEER_ANGLE, MAX_STEER_ANGLE);
	
	RearWheelPosition += CurrentVelocity.Length() * CurrentHeading * FIXED_DELTA_TIME;
	FrontWheelPosition += CurrentVelocity.Length() * CurrentHeading.RotateAngleAxis(FMath::RadiansToDegrees(CurrentSteerAngle), FVector::UpVector) * FIXED_DELTA_TIME;

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
	FVector Projection = (Temp.Dot(PathVector)/PathVector.Size()) * PathVector.GetSafeNormal() + PathStart;

	const float Alpha = FMath::Clamp((Projection - PathStart).Length() / PathVector.Length(), 0.0f, 1.0f);
	Projection = PathStart * (1 - Alpha) + PathEnd * Alpha;

	return Projection;
}

int UTrSimulationSystem::FindNearestPath(int EntityIndex, FVector& NearestProjection)
{
	NearestProjection = Positions[EntityIndex];
	int NearestPathIndex = 0;
	float SmallestDistance = TNumericLimits<float>::Max();
	
	for(int PathIndex = 0; PathIndex < PathTransforms.Num(); ++PathIndex)
	{
		const FVector Future = Positions[EntityIndex] + Velocities[EntityIndex] * LOOK_AHEAD_TIME;
		const FVector ProjectionPoint = ProjectPointOnPath(Future, PathTransforms[PathIndex].Path);
		const float Distance = FVector::Distance(ProjectionPoint, Positions[EntityIndex]);
		
		if(Distance < SmallestDistance)
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
	if(!GTrSimDebug)
	{
		return;
	}
	
	const UWorld* World = GetWorld();
	for(int Index = 0; Index < NumEntities; ++Index)
	{
		DrawDebugBox(World, Positions[Index], VEHICLE_EXTENTS, Headings[Index].ToOrientationQuat(), DebugColors[Index], false, FIXED_DELTA_TIME);
		DrawDebugDirectionalArrow(World, Positions[Index], Positions[Index] + Headings[Index] * VEHICLE_EXTENTS.X * 1.5f, 1000.0f, FColor::Red, false, FIXED_DELTA_TIME);
		DrawDebugPoint(World, Goals[Index], 2.0f, DebugColors[Index], false, FIXED_DELTA_TIME);
		DrawDebugLine(World, Positions[Index], Goals[Index], DebugColors[Index], false, FIXED_DELTA_TIME);
	}

	GEngine->AddOnScreenDebugMessage(-1, GetWorld()->GetDeltaSeconds(), FColor::Green, FString::Printf(TEXT("Speed : %.2f km/h"), Velocities[0].Length() * 0.036));
	GEngine->AddOnScreenDebugMessage(-1, GetWorld()->GetDeltaSeconds(), FColor::White, FString::Printf(TEXT("Acceleration : %.2f"), Accelerations[0]));
}

void UTrSimulationSystem::DrawInitialDebug()
{
	const UWorld* World = GetWorld();
	for(const FRpSpatialGraphNode& Node : Nodes)
	{
		for(const uint32 ConnectedNodeIndex : Node.GetConnections())
		{
			DrawDebugLine(World, Node.GetLocation(), Nodes[ConnectedNodeIndex].GetLocation(), FColor::White, true, -1);
		}
	}
}
