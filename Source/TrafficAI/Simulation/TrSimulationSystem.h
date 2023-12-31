// Copyright Anupam Sahu. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "..\Shared\TrTypes.h"
#include "Ripple/Public/RpSpatialGraphComponent.h"
#include "TrSimulationSystem.generated.h"

enum class ETrMotionState
{
	PathFollowing,
	Idle,
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

	void Initialize
		(
			const URpSpatialGraphComponent* GraphComponent,
			const TArray<FTrPath>& StartingPaths,
			TWeakPtr<TArray<FTrVehicleRepresentation>> TrafficEntities
		);
	
	void StartSimulation();

	void StopSimulation();
	
private:

	void DebugVisualization();
	
	void TickSimulation();

	void SetHeadings();
	
	void SetAccelerations();

	void UpdateVehicle();

	FVector Steer(const FVector& CurrentHeading, const FVector& TargetHeading);

private:

	int NumEntities;
	TArray<FVector> Positions;
	TArray<FVector> Velocities;
	TArray<FVector> Headings;
	TArray<FTrPath> CurrentPaths;
	TArray<float> Accelerations;
	TArray<FColor> DebugColors;
	
	FTrModelData ModelData;
	FTimerHandle SimTimerHandle;
	
	TArray<FRpSpatialGraphNode> Nodes;
};
