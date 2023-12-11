// Copyright Anupam Sahu. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "TrCommon.h"
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

	virtual void LaneInsertion();

	virtual void Drive();
	
	virtual void IntersectionHandling();
	
	virtual void Separation();

	void ApplyAcceleration();

	void GenerateRoutes();

	void AssignRoutes();
	
	float CalculateAcceleration(float CurrentSpeed, float RelativeSpeed, float CurrentGap) const;

private:

	int NumEntities;
	TArray<FVector> Positions;
	TArray<FVector> Velocities;
	TArray<FVector> Headings;
	TArray<FVector> Accelerations;
	TArray<ETrMotionState> States;
	TArray<TPair<int, int>> RouteIndices;

	FTrModelData ModelData;
	FTimerHandle SimTimerHandle;

	TArray<TArray<FVector>> Routes;

	float SimTickRate = 0.016f;
};
