// Copyright Anupam Sahu. All Rights Reserved.

#include "TrSimulationSystem.h"
#include "RpSpatialGraphComponent.h"
#include "Kismet/KismetMathLibrary.h"

constexpr float MaxSpeed = 100.0f;
constexpr float PathRadius = 100.0f;
constexpr float FixedDeltaTime = 0.016f;
constexpr float LookAheadTime = FixedDeltaTime * 100.0f;
constexpr float IntersectionRadius = 100.0f;

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
		DrawDebugDirectionalArrow(World, Positions[Index], Positions[Index] + Velocities[Index].GetSafeNormal() * 200.0f, 20.0f, FColor::Blue, false, FixedDeltaTime);
	}
}

void UTrSimulationSystem::TickSimulation()
{
	DebugVisualization();

	SetAccelerations();
	SetHeadings();
	UpdateVehicle();
}

void UTrSimulationSystem::SetHeadings()
{
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

		DrawDebugBox(GetWorld(), Future, FVector::One() * 10.0f, DebugColors[Index], false, FixedDeltaTime);
		DrawDebugSphere(GetWorld(), Projection, 10.0f, 6, DebugColors[Index], false, FixedDeltaTime);

		if(FVector::Distance(Positions[Index], Projection) > PathRadius)
		{
			Headings[Index] = Steer(Headings[Index], (Projection - Positions[Index]).GetSafeNormal());
		}
	}
}

void UTrSimulationSystem::SetAccelerations()
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
	}
}

void UTrSimulationSystem::UpdateVehicle()
{
	for(int Index = 0; Index < NumEntities; ++Index)
	{
		FVector NewVelocity = Velocities[Index] + Headings[Index].GetSafeNormal() * Accelerations[Index] * FixedDeltaTime;
		const FVector NewPosition = Positions[Index] + NewVelocity * FixedDeltaTime;
		
		Velocities[Index] = NewVelocity;
		Positions[Index] = NewPosition;
	}
}

FVector UTrSimulationSystem::Steer(const FVector& CurrentHeading, const FVector& TargetHeading)
{
	return FMath::Lerp(CurrentHeading, TargetHeading, 0.75f);
}