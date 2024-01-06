// Copyright Anupam Sahu. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "..\Shared\TrTypes.h"
#include "Ripple/Public/RpSpatialGraphComponent.h"
#include "TrSimulationSystem.generated.h"

/**
 * 
 */
UCLASS()
class TRAFFICAI_API UTrSimulationSystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	void Initialize
		(
			const URpSpatialGraphComponent* GraphComponent,
			const TArray<FTrPath>& StartingPaths,
			TWeakPtr<TArray<FTrVehicleRepresentation>> TrafficEntities
		);
	
	void StartSimulation();

	void StopSimulation();

protected:
	
	FVector ProjectEntityOnPath(int Index, const FTrPath& Path) const;

	int FindNearestPath(int EntityIndex, FVector& NearestProjection);
	
private:

	void DrawInitialDebug();

	void DebugVisualization();
	
	void TickSimulation();

	void PathFollow();

	void HandleGoal();
	
	void SetAcceleration();

	void UpdateVehicle();
	

private:

	int NumEntities;
	TArray<FVector> Positions;
	TArray<FVector> Velocities;
	TArray<FVector> Headings;
	TArray<FVector> Goals;
	TArray<float> Accelerations;
	TArray<float> SteerAngles;
	TArray<uint32> CurrentPaths; 
	TArray<FColor> DebugColors;
	
	FTrModelData ModelData;
	FTimerHandle SimTimerHandle;
	
	TArray<FTrPath> Paths;
	TArray<FRpSpatialGraphNode> Nodes;
};
