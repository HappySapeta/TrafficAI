// Copyright Anupam Sahu. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "TrafficAIRepresentationSystem.generated.h"

USTRUCT(BlueprintType)
struct TRAFFICAI_API FTrafficAISpawnRequest
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UStaticMesh* Mesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UMaterialInstance* Material;	

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<AActor> Dummy;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FTransform Transform;
};

struct TRAFFICAI_API FTrafficAIEntity
{
	UStaticMesh* Mesh = nullptr;

	int32 InstanceIndex = -1;

	AActor* Dummy = nullptr;
};

/**
 * Subsystem responsible for the visual representation of vehicles and will handle spawning of actors as well as static mesh instances.
 */
UCLASS(config = Game, DefaultConfig)
class TRAFFICAI_API UTrafficAIRepresentationSystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable)
	void SpawnDeferred(const FTrafficAISpawnRequest& SpawnRequest);

	UFUNCTION(BlueprintCallable)
	void SetFocus(const AActor* Actor) { FocusActor = Actor; }

protected:

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	virtual void Deinitialize() override;

	virtual void ProcessSpawnRequests();

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
