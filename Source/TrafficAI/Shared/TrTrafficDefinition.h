// Copyright Anupam Sahu. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "TrTrafficDefinition.generated.h"

/**
 * 
 */
UCLASS()
class TRAFFICAI_API UTrTrafficDefinition : public UDataAsset
{
	GENERATED_BODY()
	
public:
	
    UPROPERTY(EditAnywhere)
    UStaticMesh* StaticMesh;
   
    UPROPERTY(EditAnywhere)
    TSubclassOf<AActor> ActorClass;	
	
	UPROPERTY(EditAnywhere, meta = (UIMin = 0.0, UIMax = 1.0, ClampMin = 0.0, ClampMax = 1.0))
	float Ratio = 1.0f;
};
