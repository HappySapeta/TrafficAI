// Copyright Anupam Sahu. All Rights Reserved.

#include "TrafficRepresentationSystem/TrafficAIRepresentationSystem.h"
#include "TrafficRepresentationSystem/TrafficAIVisualizer.h"

void UTrafficAIRepresentationSystem::Initialize(FSubsystemCollectionBase& Collection)
{
	UWorld* World = GetWorld();
	FActorSpawnParameters SpawnParameters;
#if UE_EDITOR
	SpawnParameters.bHideFromSceneOutliner = false;
#endif
	ISMCVisualizer = World->SpawnActor<ATrafficAIVisualizer>(SpawnParameters);

	FTimerDelegate ProcessSpawnRequestsDelegate;
	ProcessSpawnRequestsDelegate.BindUObject(this, &UTrafficAIRepresentationSystem::ProcessSpawnRequests);
	World->GetTimerManager().SetTimer(SpawnTimer, ProcessSpawnRequestsDelegate, SpawnInterval, true, 1.0f);
}

void UTrafficAIRepresentationSystem::SpawnDeferred(UStaticMesh* Mesh, TSubclassOf<AActor> Dummy, const FTransform& Transform)
{
	if(ensureMsgf(Entities.Num() < MaxInstances, TEXT("[UTrafficAIRepresentationSystem::AddInstance] Spawn request was declined. Maximum capacity has been reached.")))
	{
		SpawnRequests.Push({Mesh, Dummy, Transform});
	}
}

void UTrafficAIRepresentationSystem::ProcessSpawnRequests()
{
	static FActorSpawnParameters SpawnParameters;
	
#if UE_EDITOR
	SpawnParameters.bHideFromSceneOutliner = true;
#endif

	int NumRequestsProcessed = 0;
	while(SpawnRequests.Num() > 0 && NumRequestsProcessed < BatchSize)
	{
		const FTrafficAISpawnRequest& Request = SpawnRequests[0];
		if(Entities.Num() >= MaxInstances)
		{
			break;
		}
		
		if(AActor* NewActor = GetWorld()->SpawnActor(Request.Dummy, &Request.Transform, SpawnParameters))
		{
			const int32 ISMIndex = ISMCVisualizer->AddInstance(Request.Mesh, Request.Transform); 
			Entities.Add({Request.Mesh, ISMIndex, NewActor});
		}

		SpawnRequests.RemoveAt(0);
		++NumRequestsProcessed;
	}
}

void UTrafficAIRepresentationSystem::Deinitialize()
{
	GetWorld()->GetTimerManager().ClearTimer(SpawnTimer);
}