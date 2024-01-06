// Copyright Anupam Sahu. All Rights Reserved.

#include "TrSimulationSystem.h"
#include "RpSpatialGraphComponent.h"

constexpr float MaxSpeed = 100.0f;
constexpr float FixedDeltaTime = 0.016f;
constexpr float LookAheadTime = FixedDeltaTime * 100.0f;
constexpr float PathRadius = 100.0f;
constexpr float GoalReachedDistance = 75.0f;
constexpr float ArrivalDistance = 100.0f;
constexpr float WheelBase = 100.0f;
constexpr float SteeringSpeed = 2.0f;
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

	if(GTrSimDebug)
	{
		DebugVisualization();
	}

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
		const uint32 NearestPathIndex = CurrentPaths[Index];
		FVector NearestProjection = ProjectEntityOnPath(Index, Paths[NearestPathIndex]);
		const FVector Future = Positions[Index] + Velocities[Index] * LookAheadTime;

		const float Distance = FVector::Distance(Future, NearestProjection);
		if(Distance < PathRadius)
		{
			Goals[Index] = Paths[NearestPathIndex].End;
		}
		else if(Distance >= PathRadius)
		{
			Goals[Index] = NearestProjection;
		}
	}
}

void UTrSimulationSystem::HandleGoal()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UTrSimulationSystem::HandleGoal)
	
	for(int Index = 0; Index < NumEntities; ++Index)
	{
		FTrPath& CurrentPath = Paths[CurrentPaths[Index]];
		if(FVector::PointsAreNear(Goals[Index], Positions[Index], GoalReachedDistance) && FVector::PointsAreNear(Goals[Index], CurrentPath.End, 1.0f))
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
		int LeadingVehicleIndex = -1;
		const float CurrentSpeed = Velocities[Index].Size();
		const float RelativeSpeed = LeadingVehicleIndex != -1 ? Velocities[LeadingVehicleIndex].Size() : 0.0f;
		const float CurrentGap = LeadingVehicleIndex != -1 ? FVector::Distance(Positions[LeadingVehicleIndex], Positions[Index]) : 10000.0f;
		
		const float FreeRoadTerm = ModelData.MaximumAcceleration * (1 - FMath::Pow(CurrentSpeed / ModelData.DesiredSpeed, ModelData.AccelerationExponent));

		const float DecelerationTerm = (CurrentSpeed * RelativeSpeed) / (2 * FMath::Sqrt(ModelData.MaximumAcceleration * ModelData.ComfortableBrakingDeceleration));
		const float GapTerm = (ModelData.MinimumGap + ModelData.DesiredTimeHeadWay * CurrentSpeed + DecelerationTerm) / CurrentGap;
		const float InteractionTerm = -ModelData.MaximumAcceleration * FMath::Square(GapTerm);

		const float DistanceToGoal = FVector::Distance(Positions[Index], Goals[Index]);

		float& Acceleration = Accelerations[Index]; 
		
		Acceleration = FreeRoadTerm + InteractionTerm;
		if(DistanceToGoal <= ArrivalDistance && Acceleration > 0.0f)
		{
			const float ArrivalFactor = DistanceToGoal / ArrivalDistance - 1.0f;
			Acceleration *= ArrivalFactor;
		}
		Acceleration *= DebugAccelerationScale;
	}
}

void UTrSimulationSystem::UpdateVehicle()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UTrSimulationSystem::UpdateVehicle)
	
	for(int Index = 0; Index < NumEntities; ++Index)
	{
		FVector CurrentHeading = Headings[Index];
		float& SteerAngle = SteerAngles[Index];

		//------KINEMATICS--------------------------------
		
		FVector NewVelocity = Velocities[Index] + CurrentHeading * Accelerations[Index] * FixedDeltaTime;

		const float HeadingVelocityDot = FMath::Clamp(NewVelocity.GetSafeNormal().Dot(CurrentHeading), 0.0f, 1.0f);
		NewVelocity = HeadingVelocityDot * NewVelocity.Size() * CurrentHeading;

		// Limit Speed
		if(NewVelocity.Length() > MaxSpeed)
		{
			NewVelocity = NewVelocity.GetSafeNormal() * MaxSpeed;
		}
		
		FVector NewPosition = Positions[Index] + NewVelocity * FixedDeltaTime;

		//------------------------------------------------

		//----STEER---------------------------------------

		
		FVector RearWheel = NewPosition - CurrentHeading * WheelBase * 0.5f;
		FVector FrontWheel = NewPosition + CurrentHeading * WheelBase * 0.5f;

		const FVector TargetHeading = (Goals[Index] - Positions[Index]).GetSafeNormal();
		float Delta = FMath::Atan2(CurrentHeading.X * TargetHeading.Y - CurrentHeading.Y * TargetHeading.X, CurrentHeading.X * TargetHeading.X + CurrentHeading.Y * TargetHeading.Y);

		const float MaxSteeringAngle = FMath::DegreesToRadians(40.0f);
		Delta = FMath::Clamp(Delta, -MaxSteeringAngle, +MaxSteeringAngle);

		SteerAngle += FMath::Clamp(Delta - SteerAngle, -SteeringSpeed, SteeringSpeed);
		
		RearWheel += NewVelocity * FixedDeltaTime;
		FrontWheel += NewVelocity.RotateAngleAxis(FMath::RadiansToDegrees(SteerAngle), FVector::UpVector) * FixedDeltaTime;

		const FVector NewHeading = (FrontWheel - RearWheel).GetSafeNormal();

		NewPosition = (FrontWheel + RearWheel) * 0.5f;
		NewVelocity = NewHeading * NewVelocity.Length();
		
		//-------------------------------------------------
		
		Velocities[Index] = NewVelocity;
		Positions[Index] = NewPosition;
		Headings[Index] = NewHeading;
	}
}

FVector UTrSimulationSystem::ProjectEntityOnPath(const int Index, const FTrPath& Path) const
{
	const FVector PathStart = Path.Start;
	const FVector PathEnd = Path.End;
	const FVector PathVector = PathEnd - PathStart;
		
	const FVector Future = Positions[Index] + Velocities[Index] * LookAheadTime;
	const FVector Temp = Future - PathStart;
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
		const FVector& ProjectionPoint = ProjectEntityOnPath(EntityIndex, Paths[PathIndex]);
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
	const UWorld* World = GetWorld();
	for(int Index = 0; Index < NumEntities; ++Index)
	{
		DrawDebugBox(World, Positions[Index], FVector(100.0f, 50.0f, 25.0f), Headings[Index].ToOrientationQuat(), DebugColors[Index], false, FixedDeltaTime);
		DrawDebugDirectionalArrow(World, Positions[Index], Positions[Index] + Headings[Index] * 150.0f, 200.0f, FColor::Red, false, FixedDeltaTime);
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
