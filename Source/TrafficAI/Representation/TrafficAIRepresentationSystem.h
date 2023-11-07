// Copyright Anupam Sahu. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "TrafficAICommon.h"
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

	// Material applied to the Instanced Static Meshes.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UMaterialInstance* Material;	

	// Low-detail actor entirely controlled by AI.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<AActor> Dummy;

	// Initial transform when the Entity is spawned.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FTransform Transform;
};

/**
 * Subsystem responsible for spawning Entities and handling the seamless transition of LODs.
 */
UCLASS(config = Game, DefaultConfig, DisplayName = "TrafficRepresentationSystem")
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
	TWeakPtr<TArray<FTrafficAIEntity>> GetEntities() const { return Entities; }

	class ATrafficAIMeshManager* GetTrafficVisualizer() const { return ISMCVisualizer; }

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

	TSharedPtr<TArray<FTrafficAIEntity>> Entities;

	UPROPERTY()
	TObjectPtr<class ATrafficAIMeshManager> ISMCVisualizer;

private:

	// Maximum number of Entities that can be spawned.
	UPROPERTY(Config, EditAnywhere, Category = "Representation System | Spawn Settings", meta = (TitleProperty = "Max Instances", ClampMin = 0, UIMin = 0))
	int MaxInstances = 1000;
	
	// Amount of Entities spawned in a single batch.
	UPROPERTY(Config, EditAnywhere, Category = "Representation System | Spawn Settings", meta = (TitleProperty = "Entity Spawn Batch Size", ClampMin = 1, UIMin = 1))
	uint8 SpawnBatchSize = 100;

	// Time interval before a batch of entities is spawned.
	UPROPERTY(Config, EditAnywhere, Category = "Representation System | Spawn Settings", meta = (TitleProperty = "LOD Update Interval", ClampMin = 0, UIMin = 0))
	float SpawnInterval = 0.1f;

	// Amount of Entities updated in a single batch.
	UPROPERTY(Config, EditAnywhere, Category = "Representation System | Update Settings", meta = (TitleProperty = "Entity Update Batch Size", ClampMin = 1, UIMin = 1))
	uint8 LODUpdateBatchSize = 100;

	// Time interval before the LODs of Entities are updated.
	UPROPERTY(Config, EditAnywhere, Category = "Representation System | Update Settings", meta = (TitleProperty = "LOD Update Interval", ClampMin = 0, UIMin = 0))
	float LODUpdateInterval = 0.03333f;

	// Range in which Dummies become relevant.
	UPROPERTY(Config, EditAnywhere, Category = "Representation System | Update Settings", meta = (TitleProperty = "Dummy LOD Range"))
	FFloatRange DummyRange;

	// Range in which Static Mesh Instances become relevant.
	UPROPERTY(Config, EditAnywhere, Category = "Representation System | Update Settings", meta = (TitleProperty = "Static Mesh LOD Range"))
	FFloatRange StaticMeshRange;

	// This actor's distance is used to determine the transition between LODs.
	UPROPERTY(Transient)
	const AActor* FocussedActor;
	
private:

	FTimerHandle SpawnTimer;
	FTimerHandle LODUpdateTimer;

	TArray<FTrafficAISpawnRequest> SpawnRequests;
};
