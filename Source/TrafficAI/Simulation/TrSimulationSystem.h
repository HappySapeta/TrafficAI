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

	void RegisterPath(const URpSpatialGraphComponent* GraphComponent, const TArray<TPair<uint32, uint32>>& NewPaths);

	void StartSimulation();

	void StopSimulation();

protected:

	FVector Seek(const FVector& CurrentPosition, const FVector& TargetLocation, const FVector& CurrentVelocity);
	
private:

	void DebugVisualization();
	
	void TickSimulation();

	void PathFollow();

	void UpdatePhsyics();

private:

	int NumEntities;
	TArray<FVector> Positions;
	TArray<FVector> Velocities;
	TArray<FVector> Headings;
	TArray<TPair<uint32, uint32>> Paths;
	TArray<FVector> Accelerations;
	TArray<ETrMotionState> States;

	FTrModelData ModelData;
	FTimerHandle SimTimerHandle;
	
	TArray<FRpSpatialGraphNode> Nodes;
};
