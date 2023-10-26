// Copyright Anupam Sahu. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "TrafficAIVehicle.generated.h"

class USphereComponent;
class UPhysicsConstraintComponent;

/**
 * ATrafficAIVehicle class is the base class for all vehicles in the game.
 * It is responsible for creating a vehicle setup that includes four wheels attached to a skeletal mesh,
 * with each wheel connected through physics constraints.
 * When a player takes control of the vehicle it disables all physics constraints
 * and attaches a `ChaosWheelVehicleMovementComponent` to simulate the vehicle's movement and physics behavior.
 */
UCLASS()
class TRAFFICAI_API ATrafficAIVehicle : public APawn
{
	GENERATED_BODY()

public:
	
	// Sets default values for this pawn's properties
	ATrafficAIVehicle();

private:
	
	/**
	* Sets up a wheel with the specified suffix.
	* @param Suffix A character string to identify the wheel.
	*/
	void SetupWheel(const char* Suffix);

protected:

	// The skeletal mesh component representing the vehicle.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vehicle")
	TObjectPtr<USkeletalMeshComponent> VehicleMesh;
	
	// An array of sphere components representing colliders for wheels.
	UPROPERTY(VisibleAnywhere, Category = "Wheels")
	TArray<USphereComponent*> WheelColliders;
	
	// An array of physics constraint components used for suspension in the wheels.
	UPROPERTY(VisibleAnywhere, Category = "Wheels")
	TArray<UPhysicsConstraintComponent*> SuspensionConstraints;

	// An array of physics constraint components used for restricting the rotation of wheels around the axle.
	UPROPERTY(VisibleAnywhere, Category = "Wheels")
	TArray<UPhysicsConstraintComponent*> AxisConstraints;
	
};
