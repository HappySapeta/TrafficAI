// Copyright Anupam Sahu. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RpSpatialGraphComponent.h"
#include "TrSpatialGraphComponent.generated.h"


USTRUCT()
struct FTrIntersection
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	TArray<uint32> Nodes;
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class TRAFFICAI_API UTrSpatialGraphComponent : public URpSpatialGraphComponent
{
	GENERATED_BODY()

public:

	const TArray<FTrIntersection>& GetIntersections() const { return Intersections; }
	
private:

	UPROPERTY(EditAnywhere, Category = "Intersections")
	TArray<FTrIntersection> Intersections;	
	
};
