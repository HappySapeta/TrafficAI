// Copyright Anupam Sahu. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "TrCommon.h"
#include "UObject/WeakFieldPtr.h"
#include "TrRepresentationSystem.generated.h"

// Information required to spawn an Entity.
USTRUCT(BlueprintType)
struct TRAFFICAI_API FTrafficAISpawnRequest
{
	GENERATED_BODY()

	// LOD2_Mesh used by the Instanced Static LOD2_Mesh renderer.
	// (Note - LOD2_Mesh LODs are currently not supported.)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UStaticMesh* LOD2_Mesh;

	// Low-detail actor entirely controlled by AI.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<AActor> LOD1_Actor;

	// Initial transform when the Entity is spawned.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FTransform Transform;
};

/**
 * Subsystem responsible for spawning Entities and handling the seamless transition of LODs.
 */
UCLASS(config = Game, DefaultConfig, DisplayName = "Traffic Representation System")
class TRAFFICAI_API UTrRepresentationSystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:

	UTrRepresentationSystem();
	
	// Push a request to spawn an Entity. The request is not guaranteed to be processed immediately.
	UFUNCTION(BlueprintCallable)
	void SpawnDeferred(const FTrafficAISpawnRequest& SpawnRequest);

	// Assign an actor whose distance is used for determining the appropriate Level of Detail to be used.
	UFUNCTION(BlueprintCallable)
	void SetFocus(const AActor* Actor) { POVActor = Actor; }


	// Get a weak pointer to an array of all entities.
	TWeakPtr<TArray<FTrEntity>> GetEntities() const { return Entities; }

	class ATrISMCManager* GetTrafficVisualizer() const { return ISMCManager; }

	// Reset SharedPtr to Entities.
	virtual void BeginDestroy() override;

	// Create this Subsystem only if playing in PIE or in game.
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

protected:
	// Spawn an ISMCManager and set timers.
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	// Clear all timers.
	virtual void Deinitialize() override;

private:
	
	// Process the Spawn queue in batches.
	virtual void ProcessSpawnRequests();

	// Perform LOD swaps based on the distance from the POVActor.
	virtual void UpdateLODs();

	virtual void Update();

protected:
	TSharedPtr<TArray<FTrEntity>> Entities;

	UPROPERTY()
	TObjectPtr<class ATrISMCManager> ISMCManager;

private:

	// Maximum number of Entities that can be spawned.
	UPROPERTY(Config, EditAnywhere, Category = "Representation System | Spawn Settings", meta = (TitleProperty = "Maximum number of Entities", ClampMin = 0, UIMin = 0))
	int MaxInstances = 1000;
	
	// Amount of Entities updated in a single batch.
	UPROPERTY(Config, EditAnywhere, Category = "Representation System | Update Settings", meta = (TitleProperty = "Entity Spawn & Update Batch Size", ClampMin = 1, UIMin = 1))
	uint8 ProcessingBatchSize = 100;

	// Time interval before the LODs of Entities are updated.
	UPROPERTY(Config, EditAnywhere, Category = "Representation System | Update Settings", meta = (TitleProperty = "LOD Update and Spawn Rate", ClampMin = 0, UIMin = 0))
	float TickRate = 0.03333f;

	// The range in which Actors become relevant. Since Actors have physics simulations, they are more expensive to simulate.
	UPROPERTY(Config, EditAnywhere, Category = "Representation System | Update Settings")
	FFloatRange ActorRelevancyRange;

	// The range within which Static Mesh Instances replace Actors.
	UPROPERTY(Config, EditAnywhere, Category = "Representation System | Update Settings")
	FFloatRange StaticMeshRelevancyRange;

	// This actor's distance is used to determine the transition between LODs.
	UPROPERTY()
	const AActor* POVActor;
	
private:

	FTimerHandle MainTimer;

	TArray<FTrafficAISpawnRequest> SpawnRequests;
};
