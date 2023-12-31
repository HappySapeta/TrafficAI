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

protected:

	FVector Seek(const FVector& CurrentPosition, const FVector& TargetLocation, const FVector& CurrentVelocity);
	
private:

	void DebugVisualization();
	
	void TickSimulation();

	void PathFollow();

	void ApplyForces();

private:

	int NumEntities;
	TArray<FVector> Positions;
	TArray<FVector> Velocities;
	TArray<FVector> Headings;
	TArray<FTrPath> CurrentPaths;
	TArray<FVector> Forces;
	TArray<FColor> DebugColors;
	
	FTrModelData ModelData;
	FTimerHandle SimTimerHandle;
	
	TArray<FRpSpatialGraphNode> Nodes;
};
