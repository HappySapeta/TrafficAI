// Copyright Anupam Sahu. All Rights Reserved.

#include "TrafficRepresentationSystem/TrafficAIRepresentationSystem.h"

#if UE_EDITOR
#include "Editor.h"
#endif

#include "TrafficAICommon.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "GameFramework/PlayerController.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "TrafficRepresentationSystem/TrafficAIVisualizer.h"

UTrafficAIRepresentationSystem::UTrafficAIRepresentationSystem()
{
	Entities = MakeShared<TArray<FTrafficAIEntity>>();
}

void UTrafficAIRepresentationSystem::Initialize(FSubsystemCollectionBase& Collection)
{
	UWorld* World = GetWorld();
	FActorSpawnParameters SpawnParameters;
#if UE_EDITOR
	SpawnParameters.bHideFromSceneOutliner = false;
#endif
	ISMCVisualizer = World->SpawnActor<ATrafficAIVisualizer>(SpawnParameters);

	World->GetTimerManager().SetTimer(SpawnTimer, FTimerDelegate::CreateUObject(this, &UTrafficAIRepresentationSystem::ProcessSpawnRequests), SpawnInterval, true, 1.0f);
	World->GetTimerManager().SetTimer(LODUpdateTimer, FTimerDelegate::CreateUObject(this, &UTrafficAIRepresentationSystem::UpdateLODs), UpdateInterval, true, 1.0f);
}

void UTrafficAIRepresentationSystem::SpawnDeferred(const FTrafficAISpawnRequest& SpawnRequest)
{
	if(ensureMsgf(Entities->Num() < MaxInstances, TEXT("[UTrafficAIRepresentationSystem::AddInstance] Spawn request was declined. Maximum capacity has been reached.")))
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
		if(Entities->Num() >= MaxInstances)
		{
			break;
		}
		
		if(AActor* NewActor = GetWorld()->SpawnActor(Request.Dummy, &Request.Transform, SpawnParameters))
		{
			checkf(ISMCVisualizer, TEXT("[UTrafficAIRepresentationSystem][ProcessSpawnRequests] Reference to the ISMCVisualizer is null."))
			const int32 ISMIndex = ISMCVisualizer->AddInstance(Request.Mesh, Request.Material, Request.Transform); 
			Entities->Add({Request.Mesh, ISMIndex, NewActor});
			SET_ACTOR_ENABLED(NewActor, false);
		}

		SpawnRequests.RemoveAt(0);
		++NumRequestsProcessed;
	}
}

void UTrafficAIRepresentationSystem::UpdateLODs()
{
	static int EntityIndex = 0;
	if(EntityIndex >= Entities->Num())
	{
		EntityIndex = 0;
	}

	if(!IsValid(FocussedActor))
	{
		FocussedActor = GetWorld()->GetFirstPlayerController()->GetPawn();
		if(!ensureMsgf(IsValid(FocussedActor), TEXT("[UTrafficAIRepresentationSystem::UpdateLODs] No focussed actor has been set.")))
		{
			return;
		}
	}
	
	const FVector& FocusLocation = FocussedActor->GetActorLocation();
	int CurrentBatchSize = 0;
	while(EntityIndex < Entities->Num() && CurrentBatchSize < BatchSize)
	{
		const FTrafficAIEntity& Entity = Entities->operator[](EntityIndex);
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

void UTrafficAIRepresentationSystem::BeginDestroy()
{
	Entities.Reset();
	Super::BeginDestroy();
}

bool UTrafficAIRepresentationSystem::ShouldCreateSubsystem(UObject* Outer) const
{
#if WITH_EDITOR
	return (GEditor && GEditor->IsPlaySessionInProgress());
#endif
	return true;
}

void UTrafficAIRepresentationSystem::Deinitialize()
{
	UWorld* World = GetWorld();
	World->GetTimerManager().ClearTimer(SpawnTimer);
	World->GetTimerManager().ClearTimer(LODUpdateTimer);
}
