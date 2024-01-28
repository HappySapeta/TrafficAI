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
	void StartSimulation();

	UFUNCTION(CallInEditor, BlueprintCallable)
	void StopSimulation();
	
protected:
	
	virtual void BeginPlay() override;

	void InitializeSimulator();
	
protected:

	UPROPERTY(VisibleDefaultsOnly, Category = "Configs")
	TObjectPtr<class URpSpatialGraphComponent> SpatialGraphComponent;

	UPROPERTY(EditAnywhere, Category = "Configs")
	TObjectPtr<class UTrSpawnConfiguration> SpawnConfiguration;

	UPROPERTY(EditAnywhere, Category = "Configs")
	TObjectPtr<class UTrSimulationConfiguration> SimulationConfiguration;
	
	UPROPERTY()
	TObjectPtr<class UTrRepresentationSystem> RepresentationSystem;

	UPROPERTY()
	TObjectPtr<class UTrSimulationSystem> SimulationSystem;

private:

	bool bInitialized = false;
	
};
