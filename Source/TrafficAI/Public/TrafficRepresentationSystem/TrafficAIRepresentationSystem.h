// Copyright Anupam Sahu. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "TrafficAIRepresentationSystem.generated.h"

/**
 * Actor responsible the visual representation of vehicles and will handle spawning of actors as well as static mesh instances.
 */
UCLASS(config = Game, DefaultConfig)
class TRAFFICAI_API UTrafficAIRepresentationSystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	void AddInstance(const UStaticMesh* Mesh, const TSubclassOf<AActor> Dummy, const FTransform& Transform);

protected:

	// Time interval before spawning the next batch of actors.
	UPROPERTY(Config, EditAnywhere, Category = "Representation System", meta = (TitleProperty = "Spawn Delay"))
	float SpawnDelay = 1.0f;
	
	UPROPERTY()
	TArray<AActor*> Dummies;

	UPROPERTY()
	TObjectPtr<class ATrafficAIVisualizer> Visualizer;

private:

	FTimerHandle SpawnTimer;
	
};
