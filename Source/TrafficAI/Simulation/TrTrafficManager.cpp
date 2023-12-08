// Copyright Anupam Sahu. All Rights Reserved.


#include "TrTrafficManager.h"

#include "RpSpatialGraphComponent.h"
#include "TrafficAI/Representation/TrTrafficSpawner.h"
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

	TrafficSpawner = World->GetSubsystem<UTrTrafficSpawner>();
	RepresentationSystem = World->GetSubsystem<UTrRepresentationSystem>();
	SimulationSystem = World->GetSubsystem<UTrSimulationSystem>();
	
	Super::BeginPlay();
}

void ATrTrafficManager::Spawn()
{
	check(TrafficSpawner);
	TrafficSpawner->Spawn(SpatialGraphComponent, SpawnConfiguration);
}

void ATrTrafficManager::RegisterEntities()
{
	check(RepresentationSystem);
	check(SimulationSystem);
	SimulationSystem->RegisterEntites(RepresentationSystem->GetEntities());
}

void ATrTrafficManager::StartSimulation()
{
}

void ATrTrafficManager::StopSimulation()
{
}
