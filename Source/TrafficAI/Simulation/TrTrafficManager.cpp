// Copyright Anupam Sahu. All Rights Reserved.

#include "TrTrafficManager.h"
#include "RpSpatialGraphComponent.h"
#include "TrafficAI/Representation/TrRepresentationSystem.h"
#include "TrafficAI/Simulation/TrSimulationSystem.h"
#include "TrafficAI/Utility/TrSpatialGraphComponent.h"

// Sets default values
ATrTrafficManager::ATrTrafficManager()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	SpatialGraphComponent = CreateDefaultSubobject<UTrSpatialGraphComponent>(TEXT("SpatialGraphComponent"));
}

void ATrTrafficManager::BeginPlay()
{
	const UWorld* World = GetWorld();

	bSimulate = false;
	RepresentationSystem = World->GetSubsystem<UTrRepresentationSystem>();
	SimulationSystem = World->GetSubsystem<UTrSimulationSystem>();
	
	Super::BeginPlay();
}

void ATrTrafficManager::Tick(float DeltaSeconds)
{
	if(!bSimulate)
	{
		return;
	}
	SimulationSystem->TickSimulation(DeltaSeconds);
	RepresentationSystem->UpdateLODs();
	Super::Tick(DeltaSeconds);
}

void ATrTrafficManager::SpawnVehicles()
{
	RepresentationSystem->SpawnVehiclesOnGraph(SpatialGraphComponent, SpawnConfiguration);
	SimulationSystem->Initialize(SimulationConfiguration, SpatialGraphComponent, RepresentationSystem->GetInitialTransforms(), RepresentationSystem->GetVehicleStarts());
}

void ATrTrafficManager::StartSimulation()
{
	bSimulate = true;
}

void ATrTrafficManager::StopSimulation()
{
	bSimulate = false;
}
