// Copyright Anupam Sahu. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "TrafficAIRepresentationSystem.generated.h"

// Information required to spawn an Entity.
USTRUCT(BlueprintType)
struct TRAFFICAI_API FTrafficAISpawnRequest
{
	GENERATED_BODY()

	// Mesh used by the Instanced Static Mesh renderer.
	// (Note - Mesh LODs are currently not supported.)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UStaticMesh* Mesh;

	// Material to be applied on the Instanced Static Meshes.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UMaterialInstance* Material;	

	// Low-detail actor entirely controlled by AI.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<AActor> Dummy;

	// Initial transform when the Entity is spawned.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FTransform Transform;
};

// Simulated Entity
struct TRAFFICAI_API FTrafficAIEntity
{
	UStaticMesh* Mesh = nullptr;
	
	// Index of the Instanced Static Mesh associated with the Entity.
	// This, combined with the Mesh reference, is required to uniquely identify the Entity.
	int32 InstanceIndex = -1;
	
	AActor* Dummy = nullptr;
};

/**
 * Subsystem responsible for spawning Entities and handling the seamless transition of LODs.
 */
UCLASS(config = Game, DefaultConfig)
class TRAFFICAI_API UTrafficAIRepresentationSystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	// Push a request to spawn an Entity. The request is not guaranteed to be processed immediately.
	UFUNCTION(BlueprintCallable)
	void SpawnDeferred(const FTrafficAISpawnRequest& SpawnRequest);

	// Assign an actor whose distance is used for determining the appropriate Level of Detail to be used.
	// Ideally, this would be the local player pawn.
	UFUNCTION(BlueprintCallable)
	void SetFocus(const AActor* Actor) { FocusActor = Actor; }

protected:

	// Spawn an ISMCVisualizer and set timers.
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	// Clear all timers.
	virtual void Deinitialize() override;

	// Process the Spawn queue in batches.
	virtual void ProcessSpawnRequests();

	// Perform LOD swaps based on the distance from the FocusActor.
	void UpdateLODs();
	
protected:

	TArray<FTrafficAIEntity> Entities;

	UPROPERTY()
	TObjectPtr<class ATrafficAIVisualizer> ISMCVisualizer;

private:

	// Amount of Entities updated in a single batch.
	UPROPERTY(Config, EditAnywhere, Category = "Representation System", meta = (TitleProperty = "Entity Update Batch Size"))
	uint8 BatchSize = 100;

	// Time interval before spawning the next batch of Entities.
	UPROPERTY(Config, EditAnywhere, Category = "Representation System", meta = (TitleProperty = "Spawn Interval"))
	float SpawnInterval = 0.1f;

	// Time interval before the LODs of Entities are updated.
	UPROPERTY(Config, EditAnywhere, Category = "Representation System", meta = (TitleProperty = "LOD Update Interval"))
	float UpdateInterval = 0.1f;

	// Maximum number of Entities that can be spawned.
	UPROPERTY(Config, EditAnywhere, Category = "Representation System", meta = (TitleProperty = "Max Instances"))
	int MaxInstances = 500;

	// Range in which Dummies become relevant.
	UPROPERTY(Config, EditAnywhere, Category = "Representation System", meta = (TitleProperty = "Dummy LOD Range"))
	FFloatRange DummyRange;

	// Range in which Static Mesh Instances become relevant.
	UPROPERTY(Config, EditAnywhere, Category = "Representation System", meta = (TitleProperty = "Static Mesh LOD Range"))
	FFloatRange StaticMeshRange;
	
	UPROPERTY(Transient)
	const AActor* FocusActor;
	
private:

	FTimerHandle SpawnTimer;

	FTimerHandle LODUpdateTimer;

	TArray<FTrafficAISpawnRequest> SpawnRequests;
};
