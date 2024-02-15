// Copyright Anupam Sahu. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "TrSimulationData.generated.h"

USTRUCT(BlueprintType)
struct TRAFFICAI_API FTrVehicleDynamics
{
	GENERATED_BODY()

#pragma region IDM
	
	UPROPERTY(EditAnywhere)
	float DesiredSpeed = 1000;

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

#pragma endregion

#pragma region Vehicle
	
	UPROPERTY(EditAnywhere, meta = (Units = "cm"))
	FVector Dimensions = FVector(233, 90, 72);

	UPROPERTY(EditAnywhere, meta = (Units = "cm"))
	float WheelBaseLength = 270;
	
	UPROPERTY(EditAnywhere)
	float SteeringSpeed = 1.0f;

	UPROPERTY(EditAnywhere)
	float MaxSteeringAngle = (UE_PI / 180.f) * 45.0f;
	
#pragma endregion
	
};

USTRUCT(BlueprintType)
struct TRAFFICAI_API FTrPathFollowingConfiguration
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, meta = (Units = "cm"))
	float PathFollowThreshold = 100.0f;

	UPROPERTY(EditAnywhere, meta = (Units = "cm"))
	float PathFollowOffset = 250.0f; // 300 : 3 m

	UPROPERTY(EditAnywhere, meta = (Units = "cm"))
	float LookAheadDistance = 500.0f; // 500 : 5m

	UPROPERTY(EditAnywhere, meta = (Units = "cm"))
	float GoalUpdateDistance = 500.0f; // 500 : 5m
};

/**
 * 
 */
UCLASS()
class TRAFFICAI_API UTrSimulationConfiguration : public UDataAsset
{
	GENERATED_BODY()

public:
	
	UPROPERTY(EditAnywhere, Category = "Vehicle")
	FTrVehicleDynamics VehicleConfig;

	UPROPERTY(EditAnywhere, Category = "Path Follow")
	FTrPathFollowingConfiguration PathFollowingConfig;
	
	UPROPERTY(EditAnywhere, Category = "Timings", meta = (Units = "s"))
	float TickRate = 0.016f;
};
