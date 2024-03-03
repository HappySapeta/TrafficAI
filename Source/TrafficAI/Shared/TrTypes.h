// Copyright Anupam Sahu. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectSaveContext.h"
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
	FFloatRange Separation = FFloatRange(750, 900);

	// Minimum distance from an intersection where vehicles can be spawned relative to length of the lane
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Configuration")
	float IntersectionCutoff = 500.0f;

	// Absolute Width of a lane
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Configuration")
	float LaneWidth = 250.0f;

	// Absolute Width of a lane
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Configuration")
	FVector MeshPositionOffset;

	// Traffic Archetypes defined by their LODs
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Configuration")
	TArray<FTrVehicleDefinition> VehicleVariants;
	
private:
	
	void NormalizeVariants();

public:
	
	virtual void PostSaveRoot(FObjectPostSaveRootContext ObjectSaveContext) override;
};

inline void UTrSpawnConfiguration::PostSaveRoot(FObjectPostSaveRootContext ObjectSaveContext)
{
	//NormalizeVariants();
	Super::PostSaveRoot(ObjectSaveContext);
}

inline void UTrSpawnConfiguration::NormalizeVariants()
{
	float Magnitude = 0;
	for(const FTrVehicleDefinition& Variant : VehicleVariants)
	{
		Magnitude += Variant.Ratio;
	}

	for(FTrVehicleDefinition& Variant : VehicleVariants)
	{
		Variant.Ratio /= Magnitude;
	}
}

struct TRAFFICAI_API FTrPath
{
	FVector Start = FVector::Zero();
	FVector End = FVector::Zero();
	uint32 StartNodeIndex = 0;
	uint32 EndNodeIndex = 0;

	FVector Direction() const
	{
		return (End - Start).GetSafeNormal();
	}

	float Length() const
	{
		return (End - Start).Length();
	}
};

struct TRAFFICAI_API FTrVehiclePathTransform
{
	FTransform Transform;
	FTrPath Path;
};