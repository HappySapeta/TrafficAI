// Copyright Anupam Sahu. All Rights Reserved.

#include "TrafficRepresentationSystem/TrafficAIRepresentationSystem.h"

#include "TrafficAICommon.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
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

	FTimerDelegate UpdateLODDelegate;
	UpdateLODDelegate.BindUObject(this, &UTrafficAIRepresentationSystem::UpdateLODs);
	World->GetTimerManager().SetTimer(LODUpdateTimer, UpdateLODDelegate, UpdateInterval, true, 1.0f);
}

void UTrafficAIRepresentationSystem::SpawnDeferred(const FTrafficAISpawnRequest& SpawnRequest)
{
	if(ensureMsgf(Entities.Num() < MaxInstances, TEXT("[UTrafficAIRepresentationSystem::AddInstance] Spawn request was declined. Maximum capacity has been reached.")))
	{
		SpawnRequests.Push(SpawnRequest);
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
			const int32 ISMIndex = ISMCVisualizer->AddInstance(Request.Mesh, Request.Material, Request.Transform); 
			Entities.Add({Request.Mesh, ISMIndex, NewActor});
			SET_ACTOR_ENABLED(NewActor, false);
		}

		SpawnRequests.RemoveAt(0);
		++NumRequestsProcessed;
	}
}

void UTrafficAIRepresentationSystem::UpdateLODs()
{
	static int EntityIndex = 0;
	if(EntityIndex >= Entities.Num())
	{
		EntityIndex = 0;
	}
	
	if(ensureMsgf(IsValid(FocusActor), TEXT("Focus Actor is not valid. LODs will not update.")))
	{
		const FVector& FocusLocation = FocusActor->GetActorLocation();
		int CurrentBatchSize = 0;
		while(EntityIndex < Entities.Num() && CurrentBatchSize < BatchSize)
		{
			const FTrafficAIEntity& Entity = Entities[EntityIndex];
			
			const float Distance = FVector::Distance(FocusLocation, Entity.Dummy->GetActorLocation());

			const bool bIsDummyRelevant = DummyRange.Contains(Distance);
			const bool bIsMeshRelevant = StaticMeshRange.Contains(Distance);

			SET_ACTOR_ENABLED(Entity.Dummy, bIsDummyRelevant);

			const FVector& NewScale = bIsMeshRelevant ? FVector::OneVector : FVector::ZeroVector;
			FTransform MeshTransform = Entity.Dummy->GetActorTransform();
			MeshTransform.SetScale3D(NewScale);
			
			ISMCVisualizer->GetISMC(Entity.Mesh)->UpdateInstanceTransform(Entity.InstanceIndex, MeshTransform, true, true, false);
			
			++EntityIndex;
			++CurrentBatchSize;
		}
	}
}

void UTrafficAIRepresentationSystem::Deinitialize()
{
	UWorld* World = GetWorld();
	World->GetTimerManager().ClearTimer(SpawnTimer);
	World->GetTimerManager().ClearTimer(LODUpdateTimer);
}
