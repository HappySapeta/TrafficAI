// Copyright Anupam Sahu. All Rights Reserved.

#include "TrafficAIVehicle.h"
#include "TrafficAIVehicleMovementComponent.h"

// Sets default values
ATrafficAIVehicle::ATrafficAIVehicle()
{
	PrimaryActorTick.bCanEverTick = true;

	VehicleRoot = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("VehicleRoot"));
	SetRootComponent(VehicleRoot);

	VehicleMovementComponent = CreateDefaultSubobject<UTrafficAIVehicleMovementComponent>(TEXT("ChaosWheelVehicleMovement"));
}
