// Copyright Anupam Sahu. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "TrSimulationData.h"
#include "../Shared/TrTypes.h"
#include "Ripple/Public/RpSpatialGraphComponent.h"
#include "TrSimulationSystem.generated.h"

class UTrSimulationConfiguration;

enum class ETrState
{
	PathFollowing,
	PathInserting,
	JunctionHandling,
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
	
	void Initialize
	(
		const UTrSimulationConfiguration* SimData,
		const URpSpatialGraphComponent* GraphComponent,
	    TWeakPtr<TArray<FTrVehicleRepresentation>> TrafficEntities,
	    const TArray<FTrVehiclePathTransform>& TrafficVehicleStarts
	);
	
	void StartSimulation();

	void StopSimulation();

protected:

#pragma region Utility
	
	FVector ProjectPointOnPath(const FVector& Point, const FTrPath& Path) const;

	int FindNearestPath(int EntityIndex, FVector& NearestProjection);
	
#pragma endregion

#pragma region Debug
	
	void DrawInitialDebug();

	void DebugVisualization();

#pragma endregion
	
private:

	virtual void TickSimulation();

	virtual void PathFollow();

	virtual void UpdatePath(const uint32 Index);

	virtual bool ShouldWaitAtJunction(uint32 Index);

	virtual void HandleGoal();

	virtual void SetAcceleration();

	virtual void UpdateVehicle();

	virtual void UpdateVehicleKinematics(int Index);

	virtual void UpdateVehicleSteer(int Index);

	void UpdateLeadingVehicles();

#pragma region Junctions
	
	void InitializeJunctions();

	void UpdateJunctions();

#pragma endregion

protected:

	float TickRate = 0.016f;
	float JunctionUpdateRate = 1.0f;

	FTrVehicleDynamics VehicleConfig;
	FTrPathFollowingConfiguration PathFollowingConfig;
	
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
	TArray<int> LeadingVehicleIndices;
	TArray<FColor> DebugColors;
	
	TArray<FRpSpatialGraphNode> Nodes;
	TMap<uint32, uint32> Junctions;

	FTimerHandle SimTimerHandle;
	FTimerHandle JunctionTimerHandle;
};
