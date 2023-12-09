﻿// Copyright Anupam Sahu. All Rights Reserved.

#include "TrTrafficManager.h"
#include "RpSpatialGraphComponent.h"
#include "TrafficAI/Representation/TrRepresentationSystem.h"
#include "TrafficAI/Simulation/TrSimulationSystem.h"

// Sets default values
ATrTrafficManager::ATrTrafficManager()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	SpatialGraphComponent = CreateDefaultSubobject<URpSpatialGraphComponent>(TEXT("SpatialGraphComponent"));
}

void ATrTrafficManager::BeginPlay()
{
	const UWorld* World = GetWorld();
	check(World);

	RepresentationSystem = World->GetSubsystem<UTrRepresentationSystem>();
	SimulationSystem = World->GetSubsystem<UTrSimulationSystem>();
	
	Super::BeginPlay();
}
