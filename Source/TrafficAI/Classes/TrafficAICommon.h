// Copyright Anupam Sahu. All Rights Reserved.

#pragma once
#include "TrafficAICommon.generated.h"

#define SET_ACTOR_ENABLED(Actor, Value) Actor->SetActorEnableCollision(Value); Actor->SetActorHiddenInGame(!Value); Actor->SetActorTickEnabled(Value);

// Specifies the representation of an entity at different levels of detail.
USTRUCT(BlueprintType)
struct FTrafficRepresentation
{
	GENERATED_BODY()

	// LOD 1, Farthest
	UPROPERTY(EditAnywhere)
	TObjectPtr<UStaticMesh> StaticMesh;

	// Material for StaticMesh
	UPROPERTY(EditAnywhere, meta = (EditCondition = "StaticMesh"))
	UMaterialInstance* Material;

	// LOD 0, Nearest
	UPROPERTY(EditAnywhere)
	TSubclassOf<AActor> Dummy;
};