// Copyright Anupam Sahu. All Rights Reserved.

#include "TrRepresentationSystem.h"

#if UE_EDITOR
#include "Editor.h"
#endif

#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "GameFramework/PlayerController.h"
#include "TrISMCManager.h"

UTrRepresentationSystem::UTrRepresentationSystem()
{
	Entities = MakeShared<TArray<FTrEntity>>();
	POVActor = nullptr;
}

void UTrRepresentationSystem::Initialize(FSubsystemCollectionBase& Collection)
{
	UWorld* World = GetWorld();
	FActorSpawnParameters SpawnParameters;
#if UE_EDITOR
	SpawnParameters.bHideFromSceneOutliner = false;
#endif
	ISMCManager = World->SpawnActor<ATrISMCManager>(SpawnParameters);

	World->GetTimerManager().SetTimer(MainTimer, FTimerDelegate::CreateUObject(this, &UTrRepresentationSystem::Update), TickRate, true);
}

void UTrRepresentationSystem::SpawnDeferred(const FTrafficAISpawnRequest& SpawnRequest)
{
	if(ensureMsgf(Entities->Num() < MaxInstances, TEXT("[UTrRepresentationSystem::AddInstance] Spawn request was declined. Maximum capacity has been reached.")))
	{
		SpawnRequests.Push(SpawnRequest);
	}
}

void UTrRepresentationSystem::Update()
{
	ProcessSpawnRequests();
	UpdateLODs();
}

void UTrRepresentationSystem::ProcessSpawnRequests()
{
	static FActorSpawnParameters SpawnParameters;
	
#if UE_EDITOR
	SpawnParameters.bHideFromSceneOutliner = true;
#endif

	int NumRequestsProcessed = 0;
	while(SpawnRequests.Num() > 0 && NumRequestsProcessed < ProcessingBatchSize)
	{
		const FTrafficAISpawnRequest& Request = SpawnRequests[0];
		if(Entities->Num() >= MaxInstances)
		{
			break;
		}
		
		if(AActor* NewActor = GetWorld()->SpawnActor(Request.LOD1_Actor, &Request.Transform, SpawnParameters))
		{
			checkf(ISMCManager, TEXT("[UTrRepresentationSystem][ProcessSpawnRequests] Reference to the ISMCManager is null."))
			const int32 ISMIndex = ISMCManager->AddInstance(Request.LOD2_Mesh, nullptr, Request.Transform); 
			Entities->Add({Request.LOD2_Mesh, ISMIndex, NewActor});
			SET_ACTOR_ENABLED(NewActor, false);
		}

		SpawnRequests.RemoveAt(0);
		++NumRequestsProcessed;
	}
}

void UTrRepresentationSystem::UpdateLODs()
{
	static int EntityIndex = 0;
	if(EntityIndex >= Entities->Num())
	{
		EntityIndex = 0;
	}

	if(!IsValid(POVActor))
	{
		POVActor = GetWorld()->GetFirstPlayerController()->GetPawn();
		if(!ensureMsgf(IsValid(POVActor), TEXT("[UTrRepresentationSystem::UpdateLODs] No focussed actor has been set.")))
		{
			return;
		}
	}
	
	const FVector& FocusLocation = POVActor->GetActorLocation();
	int CurrentBatchSize = 0;
	while(EntityIndex < Entities->Num() && CurrentBatchSize < ProcessingBatchSize)
	{
		const FTrEntity& Entity = Entities->operator[](EntityIndex);
		
		const float Distance = FVector::Distance(FocusLocation, Entity.Dummy->GetActorLocation());

		// Toggle Actors.
		const bool bIsActorRelevant = ActorRelevancyRange.Contains(Distance);
		SET_ACTOR_ENABLED(Entity.Dummy, bIsActorRelevant);

		// Toggle ISMCs.
		const bool bIsMeshRelevant = !bIsActorRelevant && StaticMeshRelevancyRange.Contains(Distance);
		const FVector& NewScale = bIsMeshRelevant * FVector::OneVector;
		FTransform MeshTransform = Entity.Dummy->GetActorTransform();
		MeshTransform.SetScale3D(NewScale);
			
		ISMCManager->GetISMC(Entity.Mesh)->UpdateInstanceTransform(Entity.InstanceIndex, MeshTransform, true, true, false);

		Entities->operator[](EntityIndex).LODLevel = static_cast<ELODLevel>(!bIsActorRelevant);
			
		++EntityIndex;
		++CurrentBatchSize;
	}
}

void UTrRepresentationSystem::BeginDestroy()
{
	Entities.Reset();
	Super::BeginDestroy();
}

bool UTrRepresentationSystem::ShouldCreateSubsystem(UObject* Outer) const
{
#if WITH_EDITOR
	return (GEditor && GEditor->IsPlaySessionInProgress());
#endif
	return true;
}

void UTrRepresentationSystem::Deinitialize()
{
	const UWorld* World = GetWorld();
	World->GetTimerManager().ClearTimer(MainTimer);
}
