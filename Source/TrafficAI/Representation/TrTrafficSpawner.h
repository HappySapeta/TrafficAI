// Copyright Anupam Sahu. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "TrTrafficSpawner.generated.h"

class URpSpatialGraphNode;
class URpSpatialGraphComponent;

/**
 * Configure Traffic Population Characterisics and Control overall Traffic density.
 */
USTRUCT(BlueprintType)
struct TRAFFICAI_API FTrafficSpawnConfiguration
{
	GENERATED_BODY()
	
	// Absolute Minimum distance between two spawned vehicles
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Configuration")
	float MinimumSeparation = 600.0f;

	// Variable distance between two spawned vehicles relative to length of the lane 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Configuration", meta = (UIMin = 0.0, UIMax = 1.0, ClampMin = 0.0, ClampMax = 1.0))
	float VariableSeparation = 0.25f;

	// Minimum distance from an intersection where vehicles can be spawned relative to length of the lane
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Configuration", meta = (UIMin = 0.0, UIMax = 1.0, ClampMin = 0.0, ClampMax = 1.0))
	float IntersectionCutoff = 0.1f;

	// Absolute Width of a lane
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Configuration")
	float LaneWidth = 200.0f;

	// Traffic Archetypes defined by their LODs
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Configuration")
	TArray<class UTrTrafficDefinition*> TrafficDefinitions;
};

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
	void Spawn(const URpSpatialGraphComponent* NewGraphComponent, const FTrafficSpawnConfiguration& NewRequestData);

private:

	// Traverse the edges of the Graph
	void CreateSpawnPointsOnGraph(const URpSpatialGraphComponent* GraphComponent, TArray<TArray<FTransform>>& GraphSpawnPoints);

	// Create spawn points along an edge
	void CreateSpawnPointsOnEdge(const FVector& Node1Location, const FVector& Node2Location, TArray<FTransform>& SpawnTransforms);

protected:
	
	FTrafficSpawnConfiguration SpawnRequestData;
};
