// Copyright Anupam Sahu. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "TrSimulationData.h"
#include "../Shared/TrTypes.h"
#include "Ripple/Public/RpSpatialGraphComponent.h"
#include "SpatialAcceleration/RpImplicitGrid.h"
#include "TrafficAI/Utility/TrSpatialGraphComponent.h"
#include "TrSimulationSystem.generated.h"

class UTrSimulationConfiguration;

enum class ETrState
{
	PathFollowing,
	PathInserting,
	JunctionHandling,
	None
};

class FTrIntersectionManager
{
public:

	void Initialize(const TArray<FTrIntersection>& NewIntersections)
	{
		Intersections = NewIntersections;
		for(const FTrIntersection& Intersection : NewIntersections)
		{
			for(const uint32 Node : Intersection.Nodes)
			{
				IntersectionNodes.Add(Node);
			}
		}
		
		Update();
	}

	bool IsNodeBlocked(const uint32 NodeIndex) const
	{
		if(IntersectionNodes.Contains(NodeIndex))
		{
			return !UnblockedNodes.Contains(NodeIndex);
		}

		return false;
	}

	void Update()
	{
		UnblockedNodes.Empty();
		for(const FTrIntersection& Intersection : Intersections)
		{
			uint32 RandomNode = FMath::RandRange(0, Intersection.Nodes.Num() - 1);
			UnblockedNodes.Add(Intersection.Nodes[RandomNode]);
		}
	}
	
private:

	TArray<FTrIntersection> Intersections;
	TSet<uint32> IntersectionNodes;
	TSet<uint32> UnblockedNodes;
};


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
	
	virtual void Initialize(FSubsystemCollectionBase& Collection) override {};

	virtual void TickSimulation(const float DeltaSeconds);

	void GetVehicleTransforms(TArray<FTransform>& OutTransforms, const FVector& PositionOffset);

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
	FTrIntersectionManager IntersectionManager;
	FRpImplicitGrid ImplicitGrid;

private:

	float TickRate;
	FTimerHandle IntersectionTimerHandle;
};
