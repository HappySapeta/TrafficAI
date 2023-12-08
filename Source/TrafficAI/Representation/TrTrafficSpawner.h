// Copyright Anupam Sahu. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "TrTrafficSpawner.generated.h"

class URpSpatialGraphNode;
class URpSpatialGraphComponent;

/**
 * TrTrafficSpawner traverses a Graph and spawns vehicles along its edges.
 */
UCLASS(meta = (BlueprintSpawnableComponent, DisplayName = "Traffic Spawner"), DisplayName = "Traffic Spawner")
class TRAFFICAI_API UTrTrafficSpawner : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	
	// Spawn Vehicles
	UFUNCTION(BlueprintCallable)
	void Spawn(const URpSpatialGraphComponent* NewGraphComponent, const UTrTrafficSpawnConfiguration* NewRequestData);

private:

	// Traverse the edges of the Graph
	void CreateSpawnPointsOnGraph(const URpSpatialGraphComponent* GraphComponent, TArray<TArray<FTransform>>& GraphSpawnPoints);

	// Create spawn points along an edge
	void CreateSpawnPointsOnEdge(const FVector& Node1Location, const FVector& Node2Location, TArray<FTransform>& SpawnTransforms);

protected:

	UPROPERTY()
	const class UTrTrafficSpawnConfiguration* SpawnRequestData;
	
};
