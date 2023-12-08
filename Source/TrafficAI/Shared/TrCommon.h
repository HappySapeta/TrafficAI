// Copyright Anupam Sahu. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "TrCommon.generated.h"

#define SET_ACTOR_ENABLED(Actor, Value) Actor->SetActorEnableCollision(Value); Actor->SetActorHiddenInGame(!Value); Actor->SetActorTickEnabled(Value);

enum class ELODLevel : int8
{
	None = -1,
	LOD0 = 0,
	LOD1 = 1
};

// Simulated Entity
struct FTrEntity
{
	// Mesh used for the lowest LOD.
	UStaticMesh* Mesh = nullptr;
	
	// Index of the Instanced Static Mesh associated with this Entity.
	// The InstanceIndex combined with the Mesh reference can be used to uniquely identify this Entity.
	int32 InstanceIndex = -1;

	// Actor used for the highest LOD.
	AActor* Dummy = nullptr;

	ELODLevel LODLevel;

	ELODLevel PreviousLODLevel = ELODLevel::None;
};

/**
 * 
 */
USTRUCT(BlueprintType, Blueprintable)
struct TRAFFICAI_API FTrTrafficDefinition
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere)
	UStaticMesh* StaticMesh;
   
	UPROPERTY(EditAnywhere)
	TSubclassOf<AActor> ActorClass;	
	
	UPROPERTY(EditAnywhere, meta = (UIMin = 0.0, UIMax = 1.0, ClampMin = 0.0, ClampMax = 1.0))
	float Ratio = 1.0f;
};

/**
 * Configure Traffic Population Characterisics and Control overall Traffic density.
 */
UCLASS()
class TRAFFICAI_API UTrTrafficSpawnConfiguration : public UDataAsset
{
	GENERATED_BODY()

public:
	
	// Absolute Minimum distance between two spawned vehicles
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Configuration")
	float MinimumSeparation = 600.0f;

	// Variable distance between two spawned vehicles relative to length of the lane 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Configuration", meta = (UIMin = 0.0, UIMax = 1.0, ClampMin = 0.0, ClampMax = 1.0))
	float VariableSeparation = 0.25f;

	// Minimum distance from an intersection where vehicles can be spawned relative to length of the lane
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Configuration", meta = (UIMin = 0.0, UIMax = 1.0, ClampMin = 0.0, ClampMax = 1.0))
	float IntersectionCutoff = 0.1f;

	// Absolute Width of a lane
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Configuration")
	float LaneWidth = 200.0f;

	// Traffic Archetypes defined by their LODs
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Configuration")
	TArray<FTrTrafficDefinition> TrafficDefinitions;
};