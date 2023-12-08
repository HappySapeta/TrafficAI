// Copyright Anupam Sahu. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "TrCommon.h"
#include "Subsystems/WorldSubsystem.h"
#include "TrSimulationSystem.generated.h"

/**
 * 
 */
UCLASS()
class TRAFFICAI_API UTrSimulationSystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:

	void RegisterEntites(TWeakPtr<TArray<FTrEntity>> NewEntities);

private:

	virtual void RegisterEntities_Internal();

protected:

	TWeakPtr<TArray<FTrEntity>> Entities;
	
	TArray<FVector2f> Positions;
	TArray<FVector2f> Headings;
	TArray<float> Speeds;
	
};
