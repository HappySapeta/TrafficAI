// Copyright Anupam Sahu. All Rights Reserved.

#include "TrSimulationSystem.h"
#include "RpSpatialGraphComponent.h"
#include "Kismet/KismetMathLibrary.h"

void UTrSimulationSystem::RegisterEntities(TWeakPtr<TArray<FTrVehicleRepresentation>> TrafficEntities)
{
	if(TrafficEntities.IsValid())
	{
		NumEntities = TrafficEntities.Pin()->Num();
		for(const FTrVehicleRepresentation& Entity : *TrafficEntities.Pin())
		{
			Positions.Push(Entity.Dummy->GetActorLocation());
			Velocities.Push(Entity.Dummy->GetVelocity());
			Headings.Push(Entity.Dummy->GetActorForwardVector());
			Accelerations.Push(FVector::Zero());
			States.Push(ETrMotionState::Driving);
			Waypoints.Push(Entity.InitialWaypoint);
		}
	}
}

void UTrSimulationSystem::RegisterPath(const URpSpatialGraphComponent* GraphComponent)
{
	Nodes = *GraphComponent->GetNodes();
}

void UTrSimulationSystem::StartSimulation()
{
	FTimerDelegate SimTimerDelegate;
	SimTimerDelegate.BindUObject(this, &UTrSimulationSystem::TickSimulation);

	GetWorld()->GetTimerManager().SetTimer(SimTimerHandle, SimTimerDelegate, SimTickRate, true);
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
		DrawDebugPoint(World, Positions[Index], 10.0f, FColor::Blue, false, SimTickRate);
		DrawDebugLine(World, Positions[Index], Positions[Index] + Headings[Index].GetSafeNormal() * 50.0f, FColor::White, false, SimTickRate, 0, 1.25f);
		DrawDebugLine(World, Positions[Index], Positions[Index] + Accelerations[Index].GetSafeNormal() * 50.0f, FColor::Red, false, SimTickRate, 1, 1.25f);
	}
}

void UTrSimulationSystem::TickSimulation()
{
	DebugVisualization();
	
	PathFollow();
	Drive();
	IntersectionHandling();
	ApplyAcceleration();
}

void UTrSimulationSystem::PathFollow()
{
	constexpr float IntersectionRadius = 300.0f;
	// adjust heading - TODO

	for(int Index = 0; Index < NumEntities; ++Index)
	{
		uint32& CurrentWaypointIndex = Waypoints[Index];
		FVector CurrentWaypointLocation = Nodes[CurrentWaypointIndex].GetLocation();
		if(FVector::Dist2D(Positions[Index], CurrentWaypointLocation) <= IntersectionRadius)
		{
			TSet<uint32> ConnectedIndices = Nodes[CurrentWaypointIndex].GetConnections();
			CurrentWaypointIndex = UKismetMathLibrary::RandomInteger(ConnectedIndices.Num());
			Headings[Index] = (Nodes[CurrentWaypointIndex].GetLocation() - Positions[Index]).GetSafeNormal();
		}
	}
	
	// switch to LaneInsertion or IntersectionHandling - TODO
}

void UTrSimulationSystem::Drive()
{
	// query vehicle in front and adjust acceleration
	for(int Index = 0; Index < NumEntities; ++Index)
	{
		if(States[Index] == ETrMotionState::Driving)
		{
			Accelerations[Index] = 70.0f * Headings[Index]; //CalculateAcceleration(Velocities[Index].Length(), 0.0f, 200.0f) * Headings[Index]; - TODO
		}
	}
}

void UTrSimulationSystem::IntersectionHandling()
{
	// if leading (no cars in front) - stop if red, go if green
}

void UTrSimulationSystem::ApplyAcceleration()
{
	for(int Index = 0; Index < NumEntities; ++Index)
	{
		const FVector& NewVelocity = Velocities[Index] + Accelerations[Index] * SimTickRate;
		const FVector& NewPosition = Positions[Index] + NewVelocity * SimTickRate;

		Velocities[Index] = NewVelocity;
		Positions[Index] = NewPosition;

		Accelerations[Index] = FVector::ZeroVector;
	}
}

float UTrSimulationSystem::CalculateAcceleration(const float CurrentSpeed, const float RelativeSpeed, const float CurrentGap) const
{
	const float FreeRoadTerm = ModelData.MaximumAcceleration * (1 - FMath::Pow(CurrentSpeed / ModelData.DesiredSpeed, ModelData.AccelerationExponent));

	const float DecelerationTerm = (CurrentSpeed * RelativeSpeed) / (2 * FMath::Sqrt(ModelData.MaximumAcceleration * ModelData.ComfortableBrakingDeceleration));
	const float GapTerm = (ModelData.MinimumGap + ModelData.DesiredTimeHeadWay * CurrentSpeed + DecelerationTerm) / CurrentGap;
	
	const float InteractionTerm = -ModelData.MaximumAcceleration * FMath::Square(GapTerm);

	return FreeRoadTerm + InteractionTerm; 
}
