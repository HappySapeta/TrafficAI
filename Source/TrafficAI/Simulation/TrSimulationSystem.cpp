// Copyright Anupam Sahu. All Rights Reserved.

#include "TrSimulationSystem.h"
#include "RpSpatialGraphComponent.h"

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
	}
}

void UTrSimulationSystem::TickSimulation()
{
	DebugVisualization();
}
