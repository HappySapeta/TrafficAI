// Copyright Anupam Sahu. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "..\Shared\TrTypes.h"
#include "Ripple/Public/RpSpatialGraphComponent.h"
#include "TrSimulationSystem.generated.h"

enum class ETrState
{
	PathFollowing,
	PathInserting,
	None
};

/**
 * 
 */
UCLASS()
class TRAFFICAI_API UTrSimulationSystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	
	void Initialize(const ::URpSpatialGraphComponent* GraphComponent, TWeakPtr<TArray<FTrVehicleRepresentation>> TrafficEntities, const
	                TArray<FTrVehiclePathTransform>& TrafficVehicleStarts);

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	
	void StartSimulation();

	void StopSimulation();

protected:
	
	FVector ProjectPointOnPath(const FVector& Point, const FTrPath& Path) const;

	int FindNearestPath(int EntityIndex, FVector& NearestProjection);
	
private:

	void DrawInitialDebug();

	void DebugVisualization();
	
	void TickSimulation();

	void PathFollow();
	
	void UpdatePath(const uint32 Index);

	void HandleGoal();
	
	void SetAcceleration();

	void UpdateVehicle();
	
	void UpdateVehicleKinematics(int Index);
	
	void UpdateVehicleSteer(int Index);

private:

	int NumEntities;
	TArray<FVector> Positions;
	TArray<FVector> Velocities;
	TArray<FVector> Headings;
	TArray<FVector> Goals;
	TArray<float> Accelerations;
	TArray<float> SteerAngles;
	TArray<FTrVehiclePathTransform> PathTransforms;
	TArray<ETrState> States;
	TArray<FColor> DebugColors;
	
	FTrModelData ModelData;
	FTimerHandle SimTimerHandle;
	
	TArray<FRpSpatialGraphNode> Nodes;
};
