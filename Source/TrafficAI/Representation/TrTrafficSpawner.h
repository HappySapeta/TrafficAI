// Copyright Anupam Sahu. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TrTrafficSpawner.generated.h"

class URpSpatialGraphNode;
class URpSpatialGraphComponent;

/**
 *
 */
UCLASS()
class TRAFFICAI_API UTrTrafficSpawner : public UWorldSubsystem
{
	GENERATED_BODY()

	UFUNCTION(BlueprintCallable)
	void SetTrafficGraph(const URpSpatialGraphComponent* NewGraphComponent);

	UFUNCTION(BlueprintCallable)
	void SetSpawnData(const TSubclassOf<AActor>& ActorClass, UStaticMesh* Mesh);
	
	UFUNCTION(BlueprintCallable)
	void SpawnTraffic();

private:

	void TraverseGraph();
	void CreateSpawnPointsBetweenNodes(const URpSpatialGraphNode* Node1, const URpSpatialGraphNode* Node2);
	
private:

	UPROPERTY()
	class UTrRepresentationSystem* RepresentationSystem;

	UPROPERTY()
	const URpSpatialGraphComponent* GraphComponent;

	TSubclassOf<AActor> DebugActor;
	UStaticMesh* DebugMesh;
};
