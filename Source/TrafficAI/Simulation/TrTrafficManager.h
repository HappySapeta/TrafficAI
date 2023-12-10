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

	UFUNCTION(CallInEditor, BlueprintCallable)
	void SpawnVehicles();
	
	UFUNCTION(CallInEditor, BlueprintCallable)
	void InitializeSimulator();

	UFUNCTION(CallInEditor, BlueprintCallable)
	void StartSimulation();

	UFUNCTION(CallInEditor, BlueprintCallable)
	void StopSimulation();
	
protected:
	
	virtual void BeginPlay() override;

protected:

	UPROPERTY(VisibleDefaultsOnly, Category = "Default")
	TObjectPtr<class URpSpatialGraphComponent> SpatialGraphComponent;

	UPROPERTY(EditAnywhere, Category = "Default")
	TObjectPtr<class UTrTrafficSpawnConfiguration> SpawnConfiguration;

	UPROPERTY()
	TObjectPtr<class UTrRepresentationSystem> RepresentationSystem;

	UPROPERTY()
	TObjectPtr<class UTrSimulationSystem> SimulationSystem;
};
