// Copyright Anupam Sahu. All Rights Reserved.

#include "TrVehicle.h"
#include "TrVehicleMovementComponent.h"

// Sets default values
ATrVehicle::ATrVehicle()
{
	PrimaryActorTick.bCanEverTick = true;

	VehicleRoot = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("VehicleRoot"));
	SetRootComponent(VehicleRoot);

	VehicleMovementComponent = CreateDefaultSubobject<UTrVehicleMovementComponent>(TEXT("ChaosWheelVehicleMovement"));
}
