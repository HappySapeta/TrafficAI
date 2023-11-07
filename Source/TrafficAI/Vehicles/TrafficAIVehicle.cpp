// Copyright Anupam Sahu. All Rights Reserved.

#include "TrafficAIVehicle.h"

#include "ChaosVehicleManager.h"
#include "ChaosWheeledVehicleMovementComponent.h"

// Sets default values
ATrafficAIVehicle::ATrafficAIVehicle()
{
	PrimaryActorTick.bCanEverTick = true;

	VehicleRoot = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("VehicleRoot"));
	SetRootComponent(VehicleRoot);

	ChaosMovement = CreateDefaultSubobject<UChaosWheeledVehicleMovementComponent>(TEXT("ChaosWheelVehicleMovement"));
	bIsMovementComponentEnabled = true;
}

void ATrafficAIVehicle::SetChaosEnabled(const bool bInEnable)
{
	if(!bIsMovementComponentEnabled && bInEnable)
	{
		ChaosMovement->CreatePhysicsState();
		ChaosMovement->ResetVehicleState();
		VehicleRoot->SetEnableGravity(true);
		bIsMovementComponentEnabled = true;
	}
	else if(bIsMovementComponentEnabled && !bInEnable)
	{
		ChaosMovement->DestroyPhysicsState();
		VehicleRoot->SetEnableGravity(false);
		bIsMovementComponentEnabled = false;
	}
}
