// Copyright Anupam Sahu. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TrTrafficManager.generated.h"

UCLASS()
class TRAFFICAI_API ATrTrafficManager : public AActor
{
	GENERATED_BODY()

public:
	
	ATrTrafficManager();
	
protected:
	
	virtual void BeginPlay() override;

protected:

	UPROPERTY(VisibleDefaultsOnly, Category = "Default")
	TObjectPtr<class URpSpatialGraphComponent> SpatialGraphComponent;

	UPROPERTY(EditAnywhere, Category = "Default")
	TObjectPtr<class UTrTrafficSpawnConfiguration> SpawnConfiguration;

	UPROPERTY()
	TObjectPtr<class UTrTrafficSpawner> TrafficSpawner;

	UPROPERTY()
	TObjectPtr<class UTrRepresentationSystem> RepresentationSystem;

	UPROPERTY()
	TObjectPtr<class UTrSimulationSystem> SimulationSystem;
};
