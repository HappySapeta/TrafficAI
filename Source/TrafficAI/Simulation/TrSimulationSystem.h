// Copyright Anupam Sahu. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FTrIntersectionManager.h"
#include "TrSimulationData.h"
#include "../Shared/TrTypes.h"
#include "Ripple/Public/RpSpatialGraphComponent.h"
#include "SpatialAcceleration/RpImplicitGrid.h"
#include "TrafficAI/Utility/TrSpatialGraphComponent.h"
#include "TrSimulationSystem.generated.h"

class UTrSimulationConfiguration;

UCLASS()
class TRAFFICAI_API UTrSimulationSystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	
	void Initialize
	(
		const UTrSimulationConfiguration* SimData,
		const UTrSpatialGraphComponent* GraphComponent,
		const TArray<FTrVehicleRepresentation>& TrafficEntities,
		const TArray<FTrVehiclePathTransform>& TrafficVehicleStarts
	);
	
	void Initialize(FSubsystemCollectionBase& Collection) override {};

	void TickSimulation(const float DeltaSeconds);

	void GetVehicleTransforms(TArray<FTransform>& OutTransforms, const FVector& PositionOffset);

	virtual void BeginDestroy() override;

protected:

#pragma region Utility
	
	FVector ProjectPointOnPath(const FVector& Point, const FTrPath& Path) const;

	int FindNearestPath(int EntityIndex, FVector& NearestProjection) const;

	float ScalarProjection(const FVector& V1, const FVector& V2) const { return V1.Dot(V2) / V2.Length(); }
	
#pragma endregion

#pragma region Debug
#if !UE_BUILD_SHIPPING
	void DrawDebug();
	void DrawGraph(const UWorld* World);
#endif
#pragma endregion

private:
	
	void SetGoals();
	
	void HandleGoals();

	void UpdateKinematics();

	void UpdateOrientations();

	void UpdateCollisionData();
	
	void UpdatePath(const uint32 Index);

protected:

	FTrVehicleDynamics VehicleConfig;
	FTrPathFollowingConfiguration PathFollowingConfig;
	
	int NumEntities;
	TArray<FVector> Positions;
	TArray<FVector> Velocities;
	TArray<FVector> Headings;
	TArray<FVector> Goals;
	TArray<FTrVehiclePathTransform> PathTransforms;
	TArray<int> LeadingVehicleIndices;
	TArray<bool> PathFollowingStates;

#if !UE_BUILD_SHIPPING
	TArray<FColor> DebugColors;
#endif
	
	TArray<FRpSpatialGraphNode> Nodes;
	FTrIntersectionManager IntersectionManager;
	FRpImplicitGrid ImplicitGrid;

private:

	float TickRate;
	FTimerHandle IntersectionTimerHandle;
	FTimerHandle AmberTimerHandle;
};
