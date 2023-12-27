// Copyright Anupam Sahu. All Rights Reserved.

#include "TrSimulationSystem.h"
#include "RpSpatialGraphComponent.h"

constexpr float MaxSpeed = 100.0f;
constexpr float PathRadius = 100.0f;
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
			DebugColors.Push(FColor::MakeRandomColor());
		}
	}
}

void UTrSimulationSystem::RegisterPath(const URpSpatialGraphComponent* GraphComponent, const TArray<TPair<uint32, uint32>>& StartingPaths)
{
	Nodes = *GraphComponent->GetNodes();
	Paths = StartingPaths;

	for(int Index = 0; Index < StartingPaths.Num(); ++Index)
	{
		const TPair<uint32, uint32>& Pair = StartingPaths[Index]; 
		Headings.Push((Positions[Index] - Nodes[Pair.Get<1>()].GetLocation()).GetSafeNormal());
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
		DrawDebugPoint(World, Positions[Index], 10.0f, DebugColors[Index], false, FixedDeltaTime);
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
	const float LookAheadTime = FixedDeltaTime * 100.0f;
	for(int Index = 0; Index < NumEntities; ++Index)
	{
		const FVector PathStart = Nodes[Paths[Index].Get<0>()].GetLocation();
		const FVector PathEnd = Nodes[Paths[Index].Get<1>()].GetLocation();
		const FVector Path = PathEnd - PathStart;
		
		const FVector Future = Positions[Index] + Velocities[Index] * LookAheadTime;
		const FVector Temp = Future - PathStart;
		const FVector Projection = (Temp.Dot(Path)/Path.Size()) * Path.GetSafeNormal() + PathStart;

		DrawDebugBox(GetWorld(), Future, FVector(50.0f), DebugColors[Index], false, FixedDeltaTime);
		DrawDebugSphere(GetWorld(), Projection, 50.0f, 6, DebugColors[Index], false, FixedDeltaTime);

		const float Distance = FVector::Distance(Future, Projection);
		if(Distance > PathRadius)
		{
			Velocities[Index] += Seek(Positions[Index], Projection, Velocities[Index]);
		}
		else
		{
			Velocities[Index] = Headings[Index] * MaxSpeed;
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
