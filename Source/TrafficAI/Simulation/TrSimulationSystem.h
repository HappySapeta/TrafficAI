// Copyright Anupam Sahu. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "..\Shared\TrTypes.h"
#include "Ripple/Public/RpSpatialGraphComponent.h"
#include "TrSimulationSystem.generated.h"

enum class ETrMotionState
{
	LaneInsertion,
	Driving,
	Parked,
	Intersection,
	Turning
};

/**
 * 
 */
UCLASS()
class TRAFFICAI_API UTrSimulationSystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:

	void RegisterEntities(TWeakPtr<TArray<FTrVehicleRepresentation>> TrafficEntities);

	void RegisterPath(const class URpSpatialGraphComponent* GraphComponent);

	void StartSimulation();

	void StopSimulation();
	
private:

	void DebugVisualization();
	
	void TickSimulation();

	virtual void PathFollow();
	
	virtual void Drive();
	
	virtual void IntersectionHandling();
	
	void ApplyAcceleration();
	
	float CalculateAcceleration(float CurrentSpeed, float RelativeSpeed, float CurrentGap) const;

private:

	int NumEntities;
	TArray<FVector> Positions;
	TArray<FVector> Velocities;
	TArray<FVector> Headings;
	TArray<uint32> Waypoints;
	TArray<FVector> Accelerations;
	TArray<ETrMotionState> States;

	FTrModelData ModelData;
	FTimerHandle SimTimerHandle;
	
	float SimTickRate = 0.016f;

	TArray<FRpSpatialGraphNode> Nodes;
};
