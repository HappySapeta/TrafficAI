// Copyright Anupam Sahu. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "TrSimulationData.generated.h"

/**
 * Structure representing the vehicle dynamics.
 */
USTRUCT(BlueprintType)
struct TRAFFICAI_API FTrVehicleDynamics
{
	GENERATED_BODY()

#pragma region IDM

	// The desired speed for the vehicle.
	UPROPERTY(EditAnywhere, meta = (ForceUnits = "cm/s"))
	float DesiredSpeed = 1000;

	// The MinimumGap variable defines the minimum distance required between two vehicles.
	UPROPERTY(EditAnywhere, meta = (Units = "cm"))
	float MinimumGap = 200;

	// The desired time headway between vehicles.
	UPROPERTY(EditAnywhere, meta = (Units = "s"))
	float DesiredTimeHeadWay = 1.5f;

	// Maximum forward acceleration attainable by a vehicle.
	UPROPERTY(EditAnywhere, meta = (ForceUnits = "cm/s2"))
	float MaximumAcceleration = 73.0f;

	// Maximum braking acceleration attainable by a vehicle.
	UPROPERTY(EditAnywhere, meta = (ForceUnits = "cm/s2"))
	float ComfortableBrakingDeceleration = 167.0f;
	
	UPROPERTY(EditAnywhere)
	float AccelerationExponent = 4.0f;

#pragma endregion

#pragma region Vehicle

	// Half-Dimensions of a vehicle
	UPROPERTY(EditAnywhere, meta = (Units = "cm"))
	FVector Dimensions = FVector(233, 90, 72);

	// Length of the Wheel Base
	UPROPERTY(EditAnywhere, meta = (Units = "cm"))
	float WheelBaseLength = 270;

	// Value used to scale the rate at which a vehicle steers.
	UPROPERTY(EditAnywhere)
	float SteeringSpeed = 1.0f;

	// The maximum steering angle in radians that a vehicle can achieve.
	UPROPERTY(EditAnywhere, meta = (Units = "rad"))
	float MaxSteeringAngle = (UE_PI / 180.f) * 45.0f;
	
	// The CollisionSensorRange variable determines the maximum distance at which the vehicle's collision sensor can detect obstacles
	UPROPERTY(EditAnywhere, meta = (Units = "cm"))
	float CollisionSensorRange = 2000.0f;
	
#pragma endregion
};

// This struct represents the configuration parameters for path following.
USTRUCT(BlueprintType)
struct TRAFFICAI_API FTrPathFollowingConfiguration
{
	GENERATED_BODY()

	/**
	 * The value of this variable determines the maximum distance at which the agent is considered to be "following" the path.
	 * If the agent exceeds this distance from the path, it is considered to be "off track" and may need to adjust its position.
	 */
	UPROPERTY(EditAnywhere, meta = (Units = "cm"))
	float PathFollowThreshold = 100.0f;

	/**
	 * The PathFollowOffset variable is a float variable that represents the offset used for path following.
	 * It is used in the TrSimulationSystem class for adjusting the position of vehicle relative to the path it is following.
	 * @note Increasing the value of this variable will move the vehicle further away from the path, while decreasing the value will bring it closer to the path.
	 */
	UPROPERTY(EditAnywhere, meta = (Units = "cm"))
	float PathFollowOffset = 250.0f; 

	/**
	 * The LookAheadDistance is a float variable that stores the distance that an entity should look ahead when following a path.
	 * It is used in the TrSimulationSystem class for calculating the future position of an entity and determining its goal on the path.
	 */
	UPROPERTY(EditAnywhere, meta = (Units = "cm"))
	float LookAheadDistance = 500.0f;

	/**
	 * The GoalUpdateDistance variable is a float variable that represents the maximum distance
	 * at which a vehicle is considered to have reached its goal and needs to update its path.
	 */
	UPROPERTY(EditAnywhere, meta = (Units = "cm"))
	float GoalUpdateDistance = 500.0f;
	
	// This variable determines the time interval after which all traffic signals should switch.
	UPROPERTY(EditAnywhere, meta = (Units = "s"))
	float SignalSwitchInterval = 10.0f;
};

/**
 * This struct defines the configuration options for the Implicit Grid system.
 */
USTRUCT(BlueprintType)
struct TRAFFICAI_API FTrImplicitGridConfiguration
{
	GENERATED_BODY()

	// Coverage range of the grid in the World's coordinate system.
	UPROPERTY(EditAnywhere, meta = (Units = "cm"))
	float Range = 10000.0f;

	/** 
	 * This value is used to set the number of divisions in the uniform grid.
	 * @remark Higher resolution offers more granularity at the expense of higher memory and processing costs.
	 * @remark Resolutions that are too low may also cause performance issues.
	 */
	UPROPERTY(EditAnywhere, meta = (UIMin = 1, ClampMin = 1))
	uint32 Resolution = 20;
};

// Represents the configuration for a traffic simulation.
UCLASS()
class TRAFFICAI_API UTrSimulationConfiguration : public UDataAsset
{
	GENERATED_BODY()

public:

	// The VehicleConfig class contains properties related to the configuration of a vehicle.
	UPROPERTY(EditAnywhere, Category = "Vehicle")
	FTrVehicleDynamics VehicleConfig;

	// Represents the configuration settings for path following behavior.
	UPROPERTY(EditAnywhere, Category = "Path Follow")
	FTrPathFollowingConfiguration PathFollowingConfig;

	// The configuration parameters for the spatial acceleration grid.
	UPROPERTY(EditAnywhere, Category = "Spatial Acceleration Grid")
	FTrImplicitGridConfiguration GridConfiguration;
};
