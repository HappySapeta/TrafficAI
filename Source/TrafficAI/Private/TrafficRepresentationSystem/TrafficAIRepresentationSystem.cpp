// Copyright Anupam Sahu. All Rights Reserved.

#include "TrafficRepresentationSystem/TrafficAIRepresentationSystem.h"
#include "TrafficRepresentationSystem/TrafficAIVisualizer.h"


void UTrafficAIRepresentationSystem::Initialize(FSubsystemCollectionBase& Collection)
{
	UWorld* World = GetWorld();
	if(IsValid(World))
	{
		FActorSpawnParameters SpawnParameters;
#if UE_EDITOR
		SpawnParameters.bHideFromSceneOutliner = true;
#endif
		Visualizer = World->SpawnActor<ATrafficAIVisualizer>(SpawnParameters);
	}

	Super::Initialize(Collection);
}

void UTrafficAIRepresentationSystem::AddInstance(const UStaticMesh* Mesh, const TSubclassOf<AActor> Dummy, const FTransform& Transform)
{
}
