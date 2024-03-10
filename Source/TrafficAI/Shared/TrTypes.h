// Copyright Anupam Sahu. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectSaveContext.h"
#include "TrTypes.generated.h"

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
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Configuration", meta = (Units = "cm"))
	FFloatRange Separation = FFloatRange(750, 900);

	// Minimum distance from an intersection where vehicles can be spawned relative to length of the lane
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Configuration", meta = (Units = "cm"))
	float IntersectionCutoff = 500.0f;

	// Absolute Width of a lane
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Configuration", meta = (Units = "cm"))
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

// Represents a path in a traffic simulation system.
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

/**
 * This struct is used to store the transform and path information for a traffic vehicle.
 * It is typically used in the spawning process to create instances of traffic vehicles along a specified path.
 */
struct TRAFFICAI_API FTrVehiclePathTransform
{
	FTransform Transform;
	FTrPath Path;
};