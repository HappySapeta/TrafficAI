// Copyright Anupam Sahu. All Rights Reserved.

#include "TrafficAIVehicle.h"
#include "TrVehicleMovementComponent.h"

// Sets default values
ATrafficAIVehicle::ATrafficAIVehicle()
{
	PrimaryActorTick.bCanEverTick = true;

	VehicleRoot = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("VehicleRoot"));
	SetRootComponent(VehicleRoot);

	VehicleMovementComponent = CreateDefaultSubobject<UTrVehicleMovementComponent>(TEXT("ChaosWheelVehicleMovement"));
	bIsComplexSimulationEnabled = true;
}

void ATrafficAIVehicle::SetComplexSimulationEnabled(const bool bInEnable)
{
	if(bIsComplexSimulationEnabled && !bInEnable)
	{
		VehicleMovementComponent->SetSimulationEnabled(false);
		bIsComplexSimulationEnabled = false;
	}
	else if(!bIsComplexSimulationEnabled && bInEnable)
	{
		VehicleMovementComponent->SetSimulationEnabled(true);
		bIsComplexSimulationEnabled = true;
	}
}
