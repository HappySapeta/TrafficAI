// Copyright Anupam Sahu. All Rights Reserved.

#include "TrSimulationSystem.h"
#include "RpSpatialGraphComponent.h"

const FVector VehicleExtents(500, 180, 140);
const float WheelBase = VehicleExtents.X;
constexpr float MaxSpeed = 1000.0f; // 1000 : 36 km/h
constexpr float FixedDeltaTime = 0.016f;
constexpr float LookAheadTime = FixedDeltaTime * 200.0f;
constexpr float PathRadius = 300.0f; // 300 : 3 m
constexpr float GoalRadius = 2000.0f; // 500 : 5m
constexpr float ArrivalDistance = 1000.0f; // 1000 : 1m
constexpr float SteeringSpeed = 0.5f;
constexpr float MaxSteeringAngle = (UE_PI / 180.f) * 40.0f; // 40 degrees : 0.698132 radians
constexpr float DebugAccelerationScale = 1.0f;

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

void UTrSimulationSystem::Initialize(const URpSpatialGraphComponent* GraphComponent, const TArray<FTrPath>& StartingPaths, TWeakPtr<TArray<FTrVehicleRepresentation>> TrafficEntities)
{
	if(TrafficEntities.IsValid())
	{
		NumEntities = TrafficEntities.Pin()->Num();
		
		Paths = StartingPaths;
		check(Paths.Num() > 0);

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
			CurrentPaths.Push(FindNearestPath(Index, NearestProjectionPoint));
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
	GetWorld()->GetTimerManager().SetTimer(SimTimerHandle, SimTimerDelegate, FixedDeltaTime, true);
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
	ProjectionPoints.Reserve(Paths.Num());
	
	for(int Index = 0; Index < NumEntities; ++Index)
	{
		const FVector Future = Positions[Index] + Velocities[Index] * LookAheadTime;
		const uint32 NearestPathIndex = CurrentPaths[Index];
		
		FVector NearestProjection = ProjectPointOnPath(Future, Paths[NearestPathIndex]);

		const FVector PathDirection = (Paths[NearestPathIndex].End - Paths[NearestPathIndex].Start).GetSafeNormal();
		const FVector PathRight = PathDirection.RotateAngleAxis(90.0f, FVector::UpVector);
		const FVector PathOffset = -PathRight * PathRadius;

		const FVector PositionProjection = ProjectPointOnPath(Positions[Index], Paths[NearestPathIndex]) + PathOffset;
		
		const float Distance = FVector::Distance(Positions[Index], PositionProjection);
		if(Distance < PathRadius)
		{
			Goals[Index] = Paths[NearestPathIndex].End + PathOffset;
		}
		else if(Distance >= PathRadius)
		{
			Goals[Index] = NearestProjection + PathOffset;
		}
	}
}

void UTrSimulationSystem::HandleGoal()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UTrSimulationSystem::HandleGoal)
	
	for(int Index = 0; Index < NumEntities; ++Index)
	{
		FTrPath& CurrentPath = Paths[CurrentPaths[Index]];
		if(FVector::Distance(Goals[Index], Positions[Index]) <= GoalRadius && FVector::PointsAreNear(Goals[Index], CurrentPath.End, PathRadius * 1.01f))
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
		
		const float CurrentSpeed = Velocities[Index].Size();
		const float RelativeSpeed = LeadingVehicleIndex != -1 ? Velocities[LeadingVehicleIndex].Size() : 0.0f;
		const float CurrentGap = LeadingVehicleIndex != -1 ? FVector::Distance(Positions[LeadingVehicleIndex], Positions[Index]) : TNumericLimits<float>::Max();
		
		const float FreeRoadTerm = ModelData.MaximumAcceleration * (1 - FMath::Pow(CurrentSpeed / ModelData.DesiredSpeed, ModelData.AccelerationExponent));

		const float DecelerationTerm = (CurrentSpeed * RelativeSpeed) / (2 * FMath::Sqrt(ModelData.MaximumAcceleration * ModelData.ComfortableBrakingDeceleration));
		const float GapTerm = (ModelData.MinimumGap + ModelData.DesiredTimeHeadWay * CurrentSpeed + DecelerationTerm) / CurrentGap;
		const float InteractionTerm = -ModelData.MaximumAcceleration * FMath::Square(GapTerm);

		Acceleration = FreeRoadTerm + InteractionTerm;
		Acceleration *= DebugAccelerationScale;
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

	CurrentVelocity += CurrentHeading * Accelerations[Index] * FixedDeltaTime; // v = u + a * t

	if(CurrentVelocity.Length() > MaxSpeed)
	{
		CurrentVelocity = CurrentVelocity.GetSafeNormal() * MaxSpeed;
	}
		
	CurrentPosition += CurrentVelocity * FixedDeltaTime; // x1 = x0 + v * t
}

void UTrSimulationSystem::UpdateVehicleSteer(const int Index)
{
	FVector& CurrentHeading = Headings[Index];
	FVector& CurrentPosition = Positions[Index];
	FVector& CurrentVelocity = Velocities[Index];
	float& CurrentSteerAngle = SteerAngles[Index];
	
	FVector RearWheel = CurrentPosition - CurrentHeading * WheelBase * 0.5f;
	FVector FrontWheel = CurrentPosition + CurrentHeading * WheelBase * 0.5f;

	const FVector TargetHeading = (Goals[Index] - Positions[Index]).GetSafeNormal();
	CurrentSteerAngle = FMath::Atan2
					(
						CurrentHeading.X * TargetHeading.Y - CurrentHeading.Y * TargetHeading.X,
						CurrentHeading.X * TargetHeading.X + CurrentHeading.Y * TargetHeading.Y
					);
	
	RearWheel += CurrentVelocity * FixedDeltaTime;
	FrontWheel += CurrentVelocity.RotateAngleAxis(FMath::RadiansToDegrees(CurrentSteerAngle), FVector::UpVector) * FixedDeltaTime;

	CurrentHeading = (FrontWheel - RearWheel).GetSafeNormal();
	CurrentPosition = (FrontWheel + RearWheel) * 0.5f;
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
	
	for(int PathIndex = 0; PathIndex < Paths.Num(); ++PathIndex)
	{
		const FVector Future = Positions[EntityIndex] + Velocities[EntityIndex] * LookAheadTime;
		const FVector ProjectionPoint = ProjectPointOnPath(Future, Paths[PathIndex]);
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
		DrawDebugBox(World, Positions[Index], VehicleExtents, Headings[Index].ToOrientationQuat(), DebugColors[Index], false, FixedDeltaTime);
		DrawDebugDirectionalArrow(World, Positions[Index], Positions[Index] + Headings[Index] * VehicleExtents.X * 1.5f, 1000.0f, FColor::Red, false, FixedDeltaTime);
		DrawDebugPoint(World, Goals[Index], 2.0f, DebugColors[Index], false, FixedDeltaTime);
		DrawDebugLine(World, Positions[Index], Goals[Index], DebugColors[Index], false, FixedDeltaTime);
	}
}

void UTrSimulationSystem::DrawInitialDebug()
{
	const UWorld* World = GetWorld();
	for(const FTrPath& Path : Paths)
	{
		DrawDebugLine(World, Path.Start, Path.End, FColor::White, true, -1);
	}
}
