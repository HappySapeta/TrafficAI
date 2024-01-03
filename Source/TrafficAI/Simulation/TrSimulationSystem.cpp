// Copyright Anupam Sahu. All Rights Reserved.

#include "TrSimulationSystem.h"
#include "RpSpatialGraphComponent.h"

constexpr float MaxSpeed = 100.0f;
constexpr float PathRadius = 100.0f;
constexpr float FixedDeltaTime = 0.016f;
constexpr float LookAheadTime = FixedDeltaTime * 100.0f;
constexpr float IntersectionRadius = 100.0f;
constexpr float DebugAccelerationScale = 2.0f;

void UTrSimulationSystem::Initialize(const URpSpatialGraphComponent* GraphComponent, const TArray<FTrPath>& StartingPaths, TWeakPtr<TArray<FTrVehicleRepresentation>> TrafficEntities)
{
	if(TrafficEntities.IsValid())
	{
		Nodes = *GraphComponent->GetNodes();
		CurrentPaths = StartingPaths;
		NumEntities = TrafficEntities.Pin()->Num();
		check(StartingPaths.Num() == NumEntities);
		
		for(int Index = 0; Index < NumEntities; ++Index)
		{
			const FTrVehicleRepresentation& Entity = (*TrafficEntities.Pin())[Index];
			
			Positions.Push(Entity.Dummy->GetActorLocation());
			Velocities.Push(Entity.Dummy->GetVelocity());
			Accelerations.Push(0.0f);
			Headings.Push(Entity.Dummy->GetActorForwardVector());
			Goals.Push(Nodes[CurrentPaths[Index].EndNodeIndex].GetLocation());
			SteerAngles.Push(0.0f);
			DebugColors.Push(FColor::MakeRandomColor());
		}
	}
	
	const UWorld* World = GetWorld();
	for(const FRpSpatialGraphNode& Node : Nodes)
	{
		for(const uint32 Connection : Node.GetConnections())
		{
			DrawDebugLine(World, Node.GetLocation(), Nodes[Connection].GetLocation(), FColor::White, true);
		}
	}
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

void UTrSimulationSystem::DebugVisualization()
{
	const UWorld* World = GetWorld();
	for(int Index = 0; Index < NumEntities; ++Index)
	{
		DrawDebugBox(World, Positions[Index], FVector(100.0f, 50.0f, 25.0f), Headings[Index].ToOrientationQuat(), DebugColors[Index], false, FixedDeltaTime);
		DrawDebugDirectionalArrow(World, Positions[Index], Positions[Index] + Headings[Index] * 150.0f, 200.0f, FColor::Red, false, FixedDeltaTime);
		DrawDebugPoint(World, Goals[Index], 5.0f, DebugColors[Index], false, FixedDeltaTime);
	}
}

void UTrSimulationSystem::TickSimulation()
{
	DebugVisualization();

	PathInsertion();
	SetAcceleration();
	UpdateVehicle();
}

void UTrSimulationSystem::PathInsertion()
{
	for(int Index = 0; Index < NumEntities; ++Index)
	{
		const FVector& PathStart = Nodes[CurrentPaths[Index].StartNodeIndex].GetLocation();
		const FVector& PathEnd = Nodes[CurrentPaths[Index].EndNodeIndex].GetLocation();
		const FVector Path = PathEnd - PathStart;
		
		const FVector Future = Positions[Index] + Velocities[Index] * LookAheadTime;
		const FVector Temp = Future - PathStart;
		FVector Projection = (Temp.Dot(Path)/Path.Size()) * Path.GetSafeNormal() + PathStart;

		const float Alpha = FMath::Clamp((Projection - PathStart).Length() / Path.Length(), 0.0f, 1.0f);
		Projection = PathStart * (1 - Alpha) + PathEnd * Alpha;
		
		if(FVector::Distance(Future, Projection) > PathRadius)
		{
			Goals[Index] = Projection;
		}
		else
		{
			Goals[Index] = PathEnd;
		}
	}
}

void UTrSimulationSystem::SetAcceleration()
{
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

		constexpr float ArrivalDistance = 100.0f;

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

		//------KINEMATICS--------------------------------

		//----STEER---------------------------------------

		constexpr float WheelBase = 100.0f;
		
		FVector RearWheel = NewPosition - CurrentHeading * WheelBase * 0.5f;
		FVector FrontWheel = NewPosition + CurrentHeading * WheelBase * 0.5f;

		const FVector TargetHeading = (Goals[Index] - Positions[Index]).GetSafeNormal();
		float Delta = FMath::Atan2(CurrentHeading.X * TargetHeading.Y - CurrentHeading.Y * TargetHeading.X, CurrentHeading.X * TargetHeading.X + CurrentHeading.Y * TargetHeading.Y);

		const float MaxSteeringAngle = FMath::DegreesToRadians(40.0f);
		Delta = FMath::Clamp(Delta, -MaxSteeringAngle, +MaxSteeringAngle);

		constexpr float SteeringSpeed = 2.0f;
		SteerAngle += FMath::Clamp(Delta - SteerAngle, -SteeringSpeed, SteeringSpeed);
		
		RearWheel += NewVelocity * FixedDeltaTime;
		FrontWheel += NewVelocity.RotateAngleAxis(FMath::RadiansToDegrees(SteerAngle), FVector::UpVector) * FixedDeltaTime;

		const FVector NewHeading = (FrontWheel - RearWheel).GetSafeNormal();

		NewPosition = (FrontWheel + RearWheel) * 0.5f;
		NewVelocity = NewHeading * NewVelocity.Length();
		
		//----STEER---------------------------------------
		
		Velocities[Index] = NewVelocity;
		Positions[Index] = NewPosition;
		Headings[Index] = NewHeading;
	}
}
