// Copyright Anupam Sahu. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "TrafficAICommon.h"
#include "Subsystems/WorldSubsystem.h"
#include "TrafficAISimulator.generated.h"

/**
 * 
 */
UCLASS()
class TRAFFICAI_API UTrafficAISimulator : public UWorldSubsystem
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable)
	void StartSimulation(const float TickRate, const float Acceleration, const float MaxSpeed, const FVector Location, const float
	                     MaxLoopDistance);

private:

	void DoSimulation();

private:

	TWeakPtr<TArray<FTrafficAIEntity>> Entities;

	UPROPERTY()
	class ATrafficAIVisualizer* Visualizer;
	
	FTimerHandle SimTimerHandle;

	float DeltaTime = 0.02f;
	float DesiredAcceleration = 1000;
	float DesiredMaxSpeed = 1000;
	float LoopingDistance = 2000.0f;
	
	FVector StartingLocation = FVector::ZeroVector;
	FVector PreviousVelocity = FVector::ZeroVector;
};
