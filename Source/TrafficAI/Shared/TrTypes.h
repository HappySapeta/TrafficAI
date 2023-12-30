// Copyright Anupam Sahu. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "TrTypes.generated.h"

// Simulated Entity
struct FTrVehicleRepresentation
{
	// Mesh used for the lowest LOD.
	UStaticMesh* Mesh = nullptr;
	
	// Index of the Instanced Static Mesh associated with this Entity.
	// The InstanceIndex combined with the Mesh reference can be used to uniquely identify this Entity.
	int32 InstanceIndex = -1;

	// Actor used for the highest LOD.
	AActor* Dummy = nullptr;

	// The first waypoint assigned to this entity.
	uint32 InitialWaypoint = 0;
};

/**
 * A traffic vehicle is represented by a static mesh and an actor class.
 * The ratio property determines the probability of generating this vehicle in relation to other vehicles.
 */
USTRUCT(BlueprintType, Blueprintable)
struct TRAFFICAI_API FTrVehicleDefinition
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
class TRAFFICAI_API UTrSpawnConfiguration : public UDataAsset
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
	TArray<FTrVehicleDefinition> TrafficDefinitions;
};

USTRUCT(BlueprintType)
struct TRAFFICAI_API FTrModelData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	float DesiredSpeed = 1500; // 15 m/s or 54 Km/h

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float MinimumGap = 200; // 2 m

	UPROPERTY(EditAnywhere)
	float DesiredTimeHeadWay = 1.5f; // 1.5s

	UPROPERTY(EditAnywhere)
	float MaximumAcceleration = 73.0f; // 0.73 m/s/s

	UPROPERTY(EditAnywhere)
	float ComfortableBrakingDeceleration = 167.0f; // 1.67 m/s/s

	UPROPERTY(EditAnywhere)
	float AccelerationExponent = 4.0f;
};

struct TRAFFICAI_API FTrPath
{
	FTrPath(uint32 Start, uint32 End)
		:StartNodeIndex(Start), EndNodeIndex(End)
	{}
	uint32 StartNodeIndex = 0;
	uint32 EndNodeIndex = 0;
};