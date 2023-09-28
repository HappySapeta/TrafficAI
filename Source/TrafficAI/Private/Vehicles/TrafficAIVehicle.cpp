// Copyright Anupam Sahu. All Rights Reserved.


#include "Vehicles/TrafficAIVehicle.h"


// Sets default values
ATrafficAIVehicle::ATrafficAIVehicle()
{
	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void ATrafficAIVehicle::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ATrafficAIVehicle::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// Called to bind functionality to input
void ATrafficAIVehicle::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

