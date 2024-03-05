// Copyright Anupam Sahu. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TrTrafficManager.generated.h"

/**
 * The `ATrTrafficManager` class is responsible for managing traffic simulation.
 * It provides methods for spawning vehicles, starting and stopping the simulation, and handling tick events.
 */
UCLASS()
class TRAFFICAI_API ATrTrafficManager : public AActor
{
	GENERATED_BODY()

public:
	
	ATrTrafficManager();

	/**
	 * Spawns vehicles using the Representation System,
	 * and initialize the Simulation System.
	 */
	UFUNCTION(CallInEditor, BlueprintCallable)
	void SpawnVehicles();
	
	// Starts the simulation.
	UFUNCTION(CallInEditor, BlueprintCallable)
	void StartSimulation();
	
	// Stops the simulation.
	UFUNCTION(CallInEditor, BlueprintCallable)
	void StopSimulation();
	
	virtual void Tick(float DeltaSeconds) override;
	
protected:
	
	virtual void BeginPlay() override;

protected:

	UPROPERTY(VisibleDefaultsOnly, Category = "Configs")
	TObjectPtr<class UTrSpatialGraphComponent> SpatialGraphComponent;

	UPROPERTY(EditAnywhere, Category = "Configs")
	TObjectPtr<class UTrSpawnConfiguration> SpawnConfiguration;

	UPROPERTY(EditAnywhere, Category = "Configs")
	TObjectPtr<class UTrSimulationConfiguration> SimulationConfiguration;
	
	UPROPERTY()
	TObjectPtr<class UTrRepresentationSystem> RepresentationSystem;

	UPROPERTY()
	TObjectPtr<class UTrSimulationSystem> SimulationSystem;

private:

	bool bSimulate;
	
};
