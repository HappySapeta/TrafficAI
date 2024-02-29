// Copyright Anupam Sahu. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "TrSimulationData.h"
#include "../Shared/TrTypes.h"
#include "Ripple/Public/RpSpatialGraphComponent.h"
#include "SpatialAcceleration/RpImplicitGrid.h"
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
		const TArray<FTrVehicleRepresentation>& TrafficEntities,
		const TArray<FTrVehiclePathTransform>& TrafficVehicleStarts
	);
	
	virtual void Initialize(FSubsystemCollectionBase& Collection) override {};

	virtual void TickSimulation(const float DeltaSeconds);

	void GetVehicleTransforms(TArray<FTransform>& OutTransforms);

protected:

#pragma region Utility
	
	FVector ProjectPointOnPath(const FVector& Point, const FTrPath& Path) const;

	int FindNearestPath(int EntityIndex, FVector& NearestProjection) const;

	float ScalarProjection(const FVector& V1, const FVector& V2) const { return V1.Dot(V2) / V2.Length(); }
	
#pragma endregion

#pragma region Debug
	
	void DrawFirstDebug();

	void DrawDebug();

#pragma endregion
	
private:


	virtual void SetGoals();
	
	virtual void HandleGoals();

	virtual void UpdateKinematics();

	virtual void UpdateOrientations();

	virtual void UpdateCollisionData();
	
	virtual void UpdatePath(const uint32 Index);

protected:

	FTrVehicleDynamics VehicleConfig;
	FTrPathFollowingConfiguration PathFollowingConfig;
	
	int NumEntities;
	TSharedPtr<TArray<FVector>> Positions;
	TArray<FVector> Velocities;
	TArray<FVector> Headings;
	TArray<FVector> Goals;
	TArray<FTrVehiclePathTransform> PathTransforms;
	TArray<ETrState> States;
	TArray<int> LeadingVehicleIndices;
	TArray<FColor> DebugColors;
	
	TArray<FRpSpatialGraphNode> Nodes;
	FRpImplicitGrid ImplicitGrid;

private:

	float TickRate;
	FTimerHandle JunctionTimerHandle;
};
