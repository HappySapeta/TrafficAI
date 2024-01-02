// Copyright Anupam Sahu. All Rights Reserved.

#include "TrSimulationSystem.h"
#include "RpSpatialGraphComponent.h"
#include "Kismet/KismetMathLibrary.h"

constexpr float MaxSpeed = 100.0f;
constexpr float PathRadius = 100.0f;
constexpr float FixedDeltaTime = 0.016f;
constexpr float LookAheadTime = FixedDeltaTime * 100.0f;
constexpr float IntersectionRadius = 100.0f;
constexpr float SteeringSpeed = 0.5f;
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
		DrawDebugDirectionalArrow(World, Positions[Index], Positions[Index] + Headings[Index].GetSafeNormal() * 200.0f, 20.0f, FColor::Red, false, FixedDeltaTime);
		DrawDebugPoint(World, Goals[Index], 5.0f, DebugColors[Index], false, FixedDeltaTime);
	}
}

void UTrSimulationSystem::TickSimulation()
{
	DebugVisualization();

	PathInsertion();
	SetAcceleration();
	Steer();
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

		Accelerations[Index] = FreeRoadTerm + InteractionTerm;
		Accelerations[Index] *= DebugAccelerationScale;
	}
}

void UTrSimulationSystem::UpdateVehicle()
{
	for(int Index = 0; Index < NumEntities; ++Index)
	{
		const FVector& Heading = Headings[Index].GetSafeNormal();
		FVector NewVelocity = Velocities[Index] + Heading * Accelerations[Index] * FixedDeltaTime;

		const float HeadingVelocityDot = FMath::Clamp(NewVelocity.GetSafeNormal().Dot(Heading), 0.0f, 1.0f);
		NewVelocity = HeadingVelocityDot * NewVelocity.Size() * Heading;
		
		if(NewVelocity.Length() > MaxSpeed)
		{
			NewVelocity = NewVelocity.GetSafeNormal() * MaxSpeed;
		}
		const FVector NewPosition = Positions[Index] + NewVelocity * FixedDeltaTime;
		
		Velocities[Index] = NewVelocity;
		Positions[Index] = NewPosition;
	}
}

void UTrSimulationSystem::Steer()
{
	for(int Index = 0; Index < NumEntities; ++Index)
	{
		FVector& CurrentHeading = Headings[Index];
		const FVector TargetHeading = (Goals[Index] - Positions[Index]).GetSafeNormal();
		float Delta = FMath::Atan2(CurrentHeading.X * TargetHeading.Y - CurrentHeading.Y * TargetHeading.X, CurrentHeading.X * TargetHeading.X + CurrentHeading.Y * TargetHeading.Y);
		
		Delta = FMath::Clamp(Delta, -PI/4, PI/4);

		CurrentHeading = CurrentHeading.RotateAngleAxis(Delta * SteeringSpeed, FVector::UpVector);
	}
}