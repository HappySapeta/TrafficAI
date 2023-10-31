// Copyright Anupam Sahu. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TrafficRepresentationSystem/TrafficAIRepresentationSystem.h"
#include "TrafficAISimulationSystem.generated.h"

USTRUCT(BlueprintType)
struct FSimulationModel
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	float DesiredSpeed = 3000;

	UPROPERTY(EditAnywhere)
	float MinimumGap = 200;

	UPROPERTY(EditAnywhere)
	float DesiredTimeHeadWay = 1.5f;

	UPROPERTY(EditAnywhere)
	float MaximumAcceleration = 73.0f;

	UPROPERTY(EditAnywhere)
	float ComfortableBrakingDeceleration = 167.0f;

	UPROPERTY(EditAnywhere)
	float AccelerationExponent = 4.0f;

	UPROPERTY(EditAnywhere)
	float SimulationSpeed = 1.0f;
};

/**
 *
 */
UCLASS(Config = Game, DefaultConfig)
class TRAFFICAI_API UTrafficAISimulationSystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable)
	void StartSimulation();

private:

	float IDM_Acceleration(float CurrentSpeed, float RelativeSpeed, float CurrentGap) const;

	void Simulate() const;

private:

	TWeakPtr<TArray<FTrafficAIEntity>, ESPMode::ThreadSafe> Entities;
	
	UPROPERTY()
	UTrafficAIRepresentationSystem* RepSystem;

	UPROPERTY(Config, EditAnywhere, Category = "Simulation System | Update", meta = (TitleProperty = "Simulation Tick", ClampMin = 0.01, UIMin = 0.01))
	float TickRate = 0.01f;

	UPROPERTY(Config, EditAnywhere, Category = "Simulation System | Update", meta = (TitleProperty = "Look Ahead Distance", ClampMin = 1, UIMin = 1))
	float LookAheadDistance = 1000.0f;

	UPROPERTY(Config, EditAnywhere, Category = "Simulation System | Update", meta = (TitleProperty = "Base Acceleration", ClampMin = 1, UIMin = 1))
	float BaseAcceleration = 10000.0f;

	FSimulationModel SimulationModel;
	
	FTimerHandle SimTimerHandle;
	
};
