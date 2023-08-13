// Copyright Anupam Sahu. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "TrafficAIRepresentationSystem.generated.h"

struct TRAFFICAI_API FTrafficAISpawnRequest
{
	UStaticMesh* Mesh = nullptr;
	
	TSubclassOf<AActor> Dummy = nullptr;
	
	FTransform Transform = FTransform::Identity;
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
	virtual void SpawnDeferred(UStaticMesh* Mesh, TSubclassOf<AActor> Dummy, const FTransform& Transform);

protected:

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	virtual void Deinitialize() override;

	virtual void ProcessSpawnRequests();
	
protected:
	
	TArray<FTrafficAIEntity> Entities;
	
	UPROPERTY()
	TObjectPtr<class ATrafficAIVisualizer> ISMCVisualizer;

private:

	// Amount of Entities spawned in a single batch.
	UPROPERTY(Config, EditAnywhere, Category = "Representation System", meta = (TitleProperty = "Spawn Batch Size"))
	uint8 BatchSize = 100;

	// Time interval before spawning the next batch of Entities.
	UPROPERTY(Config, EditAnywhere, Category = "Representation System", meta = (TitleProperty = "Spawn Interval"))
	float SpawnInterval = 0.1f;

	// Maximum number of Entities that can be spawned.
	UPROPERTY(Config, EditAnywhere, Category = "Representation System", meta = (TitleProperty = "Max Instances"))
	int MaxInstances = 500;
	
private:

	FTimerHandle SpawnTimer;

	TArray<FTrafficAISpawnRequest> SpawnRequests;
};
