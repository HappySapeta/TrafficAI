// Copyright Anupam Sahu. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "UObject/WeakFieldPtr.h"
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
	// Mesh used for the lowest LOD.
	UStaticMesh* Mesh = nullptr;
	
	// Index of the Instanced Static Mesh associated with this Entity.
	// The InstanceIndex combined with the Mesh reference can be used to uniquely identify this Entity.
	int32 InstanceIndex = -1;

	// Actor used for highest the highest LOD.
	AActor* Dummy = nullptr;
};

/**
 * Subsystem responsible for spawning Entities and handling the seamless transition of LODs.
 */
UCLASS(config = Game, DefaultConfig)
class TRAFFICAI_API UTrafficAIRepresentationSystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:

	UTrafficAIRepresentationSystem();
	
	// Push a request to spawn an Entity. The request is not guaranteed to be processed immediately.
	UFUNCTION(BlueprintCallable)
	void SpawnDeferred(const FTrafficAISpawnRequest& SpawnRequest);

	// Assign an actor whose distance is used for determining the appropriate Level of Detail to be used.
	UFUNCTION(BlueprintCallable)
	void SetFocus(const AActor* Actor) { FocussedActor = Actor; }

	// Get a weak pointer to an array of all entities.
	TWeakPtr<TArray<FTrafficAIEntity>, ESPMode::ThreadSafe> GetEntities() { return Entities; }

	// Reset SharedPtr to Entities.
	virtual void BeginDestroy() override;

	// Create this Subsystem only if playing in PIE or in game.
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	
protected:

	// Spawn an ISMCVisualizer and set timers.
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	// Clear all timers.
	virtual void Deinitialize() override;

	// Process the Spawn queue in batches.
	virtual void ProcessSpawnRequests();

	// Perform LOD swaps based on the distance from the FocussedActor.
	void UpdateLODs();

protected:

	TSharedPtr<TArray<FTrafficAIEntity>, ESPMode::ThreadSafe> Entities;

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

	// This actor's distance is used to determine the transition between LODs.
	UPROPERTY(Transient)
	const AActor* FocussedActor;
	
private:

	FTimerHandle SpawnTimer;
	FTimerHandle LODUpdateTimer;

	TArray<FTrafficAISpawnRequest> SpawnRequests;
};
