// Copyright Anupam Sahu. All Rights Reserved.

#include "TrSimulationSystem.h"
#include "RpSpatialGraphComponent.h"

constexpr float MaxSpeed = 100.0f;
constexpr float PathRadius = 100.0f;
constexpr float FixedDeltaTime = 0.016f;
constexpr float IntersectionRadius = 100.0f;

void UTrSimulationSystem::Initialize
	(
		const URpSpatialGraphComponent* GraphComponent,
		const TArray<FTrPath>& StartingPaths,
		TWeakPtr<TArray<FTrVehicleRepresentation>> TrafficEntities
	)
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
			Accelerations.Push(FVector::Zero());
			Headings.Push(FVector::Zero());
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
		DrawDebugPoint(World, Positions[Index], 10.0f, DebugColors[Index], false, FixedDeltaTime);
	}
}

void UTrSimulationSystem::TickSimulation()
{
	DebugVisualization();
	
	PathInsertion();
	PathFollow();
	IntersectionHandling();
	UpdateKinematics();
}

void UTrSimulationSystem::PathFollow()
{
	for(int Index = 0; Index < NumEntities; ++Index)
	{
		const FVector PathStart = Nodes[CurrentPaths[Index].StartNodeIndex].GetLocation();
		const FVector PathEnd = Nodes[CurrentPaths[Index].EndNodeIndex].GetLocation();
		const FVector Path = PathEnd - PathStart;
		const FVector PathLeft = Path.GetSafeNormal().RotateAngleAxis(-90.0, FVector::UpVector);

		const FVector Target = PathEnd + PathLeft * PathRadius;

		Accelerations[Index] += Seek(Positions[Index], Target, Velocities[Index]);
	}
}

void UTrSimulationSystem::PathInsertion()
{
	const float LookAheadTime = FixedDeltaTime * 100.0f;
	for(int Index = 0; Index < NumEntities; ++Index)
	{
		const FVector PathStart = Nodes[CurrentPaths[Index].StartNodeIndex].GetLocation();
		const FVector PathEnd = Nodes[CurrentPaths[Index].EndNodeIndex].GetLocation();
		const FVector Path = PathEnd - PathStart;
		
		const FVector Future = Positions[Index] + Velocities[Index] * LookAheadTime;
		const FVector Temp = Future - PathStart;
		FVector Projection = (Temp.Dot(Path)/Path.Size()) * Path.GetSafeNormal() + PathStart;
		FVector PathLeft = Path.GetSafeNormal().RotateAngleAxis(-90.0, FVector::UpVector);
		Projection = Projection + PathLeft * PathRadius;
		
		//DrawDebugBox(GetWorld(), Future, FVector(50.0f), DebugColors[Index], false, FixedDeltaTime);
		//DrawDebugSphere(GetWorld(), Projection, 50.0f, 6, DebugColors[Index], false, FixedDeltaTime);

		const float Distance = FVector::Distance(Future, Projection);
		if(Distance > PathRadius)
		{
			Accelerations[Index] += Seek(Positions[Index], Projection, Velocities[Index]);
		}
	}
}

void UTrSimulationSystem::IntersectionHandling()
{
	for(int Index = 0; Index < NumEntities; ++Index)
	{
		const FVector PathStart = Nodes[CurrentPaths[Index].StartNodeIndex].GetLocation();
		const FVector PathEnd = Nodes[CurrentPaths[Index].EndNodeIndex].GetLocation();
		const FVector Path = PathEnd - PathStart;
		const FVector PathLeft = Path.GetSafeNormal().RotateAngleAxis(-90.0, FVector::UpVector);

		const FVector Target = PathEnd + PathLeft * PathRadius;
		if(FVector::Distance(Positions[Index], Target) < IntersectionRadius)
		{
			for(const uint32 ConnectionIndex : Nodes[CurrentPaths[Index].EndNodeIndex].GetConnections())
			{
				if(ConnectionIndex != CurrentPaths[Index].StartNodeIndex)
				{
					CurrentPaths[Index].StartNodeIndex = CurrentPaths[Index].EndNodeIndex;
					CurrentPaths[Index].EndNodeIndex = ConnectionIndex;
					break;
				}
			}
		}
	}
}

FVector UTrSimulationSystem::Seek(const FVector& CurrentPosition, const FVector& TargetLocation, const FVector& CurrentVelocity)
{
	const FVector& DesiredVelocity = (TargetLocation - CurrentPosition).GetSafeNormal() * MaxSpeed;
	return (DesiredVelocity - CurrentVelocity) / FixedDeltaTime;
}

void UTrSimulationSystem::UpdateKinematics()
{
	for(int Index = 0; Index < NumEntities; ++Index)
	{
		const FVector NewVelocity = Velocities[Index] + Accelerations[Index] * FixedDeltaTime;
		const FVector NewPosition = Positions[Index] + NewVelocity * FixedDeltaTime;

		Accelerations[Index] = FVector::Zero();

		Velocities[Index] = NewVelocity;
		Positions[Index] = NewPosition;
	}
}