// Copyright Anupam Sahu. All Rights Reserved.

#include "TrRepresentationSystem.h"

#if UE_EDITOR
#include "Editor.h"
#endif

#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "TrISMCManager.h"
#include "GameFramework/PlayerController.h"
#include "DeferredBatchProcessor/RpDeferredBatchProcessingSystem.h"

UTrRepresentationSystem::UTrRepresentationSystem()
{
	Entities = MakeShared<TArray<FTrEntity>>();
	POVActor = nullptr;
}

bool UTrRepresentationSystem::ShouldCreateSubsystem(UObject* Outer) const
{
#if WITH_EDITOR
	return (GEditor && GEditor->IsPlaySessionInProgress());
#endif
	return true;
}

void UTrRepresentationSystem::Initialize(FSubsystemCollectionBase& Collection)
{
	UWorld* World = GetWorld();
	ISMCManager = World->SpawnActor<ATrISMCManager>();
}

void UTrRepresentationSystem::PostInitialize()
{
	InitializeLODUpdater();
	Super::PostInitialize();
}

void UTrRepresentationSystem::InitializeLODUpdater()
{
	// TODO : Can we completely rely on the Batch Processor to do the LOD update instead of using custom batching logic ?
	URpDeferredBatchProcessingSystem* BatchProcessor = GetWorld()->GetSubsystem<URpDeferredBatchProcessingSystem>();
	if(ensureMsgf(BatchProcessor, TEXT("BatchProcessor not found.")))
	{
		BatchProcessor->QueueCommand("LODUpdateProcessor", [this]()
		{
			TRACE_CPUPROFILER_EVENT_SCOPE(UTrRepresentationSystem::UpdateLODLambda)
			
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
		});
	}
}

void UTrRepresentationSystem::SpawnDeferred(const FTrafficAISpawnRequest& SpawnRequest)
{
	URpDeferredBatchProcessingSystem* BatchProcessor = GetWorld()->GetSubsystem<URpDeferredBatchProcessingSystem>();
	if(ensure(BatchProcessor))
	{
		BatchProcessor->QueueCommand("SpawnProcessor", [this, SpawnRequest]()
		{
			static FActorSpawnParameters SpawnParameters;
#if UE_EDITOR
			SpawnParameters.bHideFromSceneOutliner = true;
#endif
			if(AActor* NewActor = GetWorld()->SpawnActor(SpawnRequest.LOD1_Actor, &SpawnRequest.Transform, SpawnParameters))
			{
				checkf(ISMCManager, TEXT("[UTrRepresentationSystem][ProcessSpawnRequests] Reference to the ISMCManager is null."))
				const int32 ISMIndex = ISMCManager->AddInstance(SpawnRequest.LOD2_Mesh, nullptr, SpawnRequest.Transform); 
				Entities->Add({SpawnRequest.LOD2_Mesh, ISMIndex, NewActor});
				SET_ACTOR_ENABLED(NewActor, false);
			}
		});
	}
}

void UTrRepresentationSystem::BeginDestroy()
{
	Entities.Reset();
	Super::BeginDestroy();
}

void UTrRepresentationSystem::Deinitialize()
{
	const UWorld* World = GetWorld();
	World->GetTimerManager().ClearTimer(MainTimer);
}
