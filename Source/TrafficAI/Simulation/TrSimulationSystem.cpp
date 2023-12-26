// Copyright Anupam Sahu. All Rights Reserved.

#include "TrSimulationSystem.h"
#include "RpSpatialGraphComponent.h"

constexpr float MaxSpeed = 100.0f;
constexpr float PathRadius = 200.0f;
constexpr float FixedDeltaTime = 0.016f;

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
		}
	}
}

void UTrSimulationSystem::RegisterPath(const URpSpatialGraphComponent* GraphComponent, const TArray<TPair<uint32, uint32>>& NewPaths)
{
	Nodes = *GraphComponent->GetNodes();
	Paths = NewPaths;
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
		DrawDebugPoint(World, Positions[Index], 10.0f, FColor::Blue, false, FixedDeltaTime);
	}
}

void UTrSimulationSystem::TickSimulation()
{
	DebugVisualization();

	PathFollow();
	UpdatePhsyics();
}

void UTrSimulationSystem::PathFollow()
{
	for(int Index = 0; Index < NumEntities; ++Index)
	{
		const FVector Path = Nodes[Paths[Index].Get<1>()].GetLocation() - Nodes[Paths[Index].Get<0>()].GetLocation();
		const FVector Future = Positions[Index] + Velocities[Index] * FixedDeltaTime;
		const FVector Projection = ((Future - Nodes[Paths[Index].Get<0>()].GetLocation()).Dot(Path) / Path.Length()) * Path.GetSafeNormal();

		const float Distance = FVector::Distance(Future, Projection);
		if(Distance > PathRadius)
		{
			Velocities[Index] += Seek(Positions[Index], Projection, Velocities[Index]);
		}
	}
}

void UTrSimulationSystem::UpdatePhsyics()
{
	for(int Index = 0; Index < NumEntities; ++Index)
	{
		const FVector NewVelocity = Velocities[Index] + Accelerations[Index] * FixedDeltaTime;
		const FVector NewPosition = Positions[Index] + NewVelocity * FixedDeltaTime;

		Velocities[Index] = NewVelocity;
		Positions[Index] = NewPosition;
	}
}

FVector UTrSimulationSystem::Seek(const FVector& CurrentPosition, const FVector& TargetLocation, const FVector& CurrentVelocity)
{
	const FVector& DesiredVelocity = (TargetLocation - CurrentPosition).GetSafeNormal() * MaxSpeed;

	return DesiredVelocity - CurrentVelocity;
}
