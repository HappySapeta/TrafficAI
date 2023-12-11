// Copyright Anupam Sahu. All Rights Reserved.

#include "TrRepresentationSystem.h"

#if UE_EDITOR
#include "Editor.h"
#endif

#include "TrUtility.h"
#include "TrISMCManager.h"

#include "RpSpatialGraphComponent.h"
#include "DeferredBatchProcessor/RpDeferredBatchProcessingSystem.h"

#include "Kismet/KismetMathLibrary.h"
#include "GameFramework/PlayerController.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"

UTrRepresentationSystem::UTrRepresentationSystem()
{
	Entities = MakeShared<TArray<FTrVehicleRepresentation>>();
	POVActor = nullptr;
}

void UTrRepresentationSystem::Spawn(const URpSpatialGraphComponent* NewGraphComponent, const UTrSpawnConfiguration* NewSpawnConfiguration)
{
	if(IsValid(NewGraphComponent))
	{
		TArray<TArray<FTransform>> GraphPoints;
		Spawner.CreateSpawnPointsOnGraph(NewGraphComponent, GraphPoints, NewSpawnConfiguration);
		
		for(const TArray<FTransform>& Edge : GraphPoints)
		{
			for(const FTransform& SpawnTransform : Edge)
			{
				FTrafficAISpawnRequest NewSpawnRequest;
				NewSpawnRequest.Transform = SpawnTransform;
				NewSpawnRequest.LOD1_Actor = NewSpawnConfiguration->TrafficDefinitions[0].ActorClass;
				NewSpawnRequest.LOD2_Mesh = NewSpawnConfiguration->TrafficDefinitions[0].StaticMesh;
				
				SpawnDeferred(NewSpawnRequest);
			}
		}
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
				const FTrVehicleRepresentation& Entity = Entities->operator[](EntityIndex);
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
				
				++EntityIndex;
				++CurrentBatchSize;
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

void FTrSpawner::CreateSpawnPointsOnGraph(const URpSpatialGraphComponent* GraphComponent, TArray<TArray<FTransform>>& GraphSpawnPoints, const UTrSpawnConfiguration* SpawnConfiguration)
{
	check(SpawnConfiguration);
	
	const TArray<FRpSpatialGraphNode>* Nodes = GraphComponent->GetNodes();
	TSet<TPair<uint32, uint32>> EdgeSet;

	uint32 NumNodes = static_cast<uint32>(Nodes->Num());
	for(uint32 Index = 0; Index < NumNodes; ++Index)
	{
		TSet<uint32> ConnectedIndices = Nodes->operator[](Index).GetConnections();
		for(uint32 ConnectedIndex : ConnectedIndices)
		{
			if(EdgeSet.Contains({Index, ConnectedIndex}) || EdgeSet.Contains({ConnectedIndex, Index}))
			{
				continue;
			}

			TArray<FTransform> SpawnPoints;
			const FVector& Node1Location = Nodes->operator[](Index).GetLocation();
			const FVector& Node2Location = Nodes->operator[](ConnectedIndex).GetLocation();
			
			CreateSpawnPointsOnEdge(Node1Location, Node2Location, SpawnPoints, SpawnConfiguration);
			CreateSpawnPointsOnEdge(Node2Location, Node1Location, SpawnPoints, SpawnConfiguration);
			
			GraphSpawnPoints.Push(SpawnPoints);
			
			EdgeSet.Add({Index, ConnectedIndex});
			EdgeSet.Add({ConnectedIndex, Index});
		}
	}
}

void FTrSpawner::CreateSpawnPointsOnEdge(const FVector& Node1Location, const FVector& Node2Location, TArray<FTransform>& SpawnTransforms, const UTrSpawnConfiguration* SpawnConfiguration)
{
	const float EdgeLength = FVector::Distance(Node1Location, Node2Location);
	const float NormalizedMinimumSeparation = (SpawnConfiguration->MinimumSeparation) / EdgeLength;
	
	float TMin = SpawnConfiguration->IntersectionCutoff;
	while(TMin < 1.0f - SpawnConfiguration->IntersectionCutoff)
	{
		float TMax = TMin + (1 - FMath::Clamp(SpawnConfiguration->VariableSeparation, 0.0f, 1.0f));
		float T = FMath::Clamp(FMath::RandRange(TMin, TMax), 0.0f, 1.0f);
		
		TMin = T + NormalizedMinimumSeparation;
		
		const FVector NewForwardVector = (Node2Location - Node1Location).GetSafeNormal();
		const FRotator NewRotation = UKismetMathLibrary::MakeRotFromX(NewForwardVector);
		const FVector NewRightVector = NewForwardVector.Cross(FVector::UpVector);
		const FVector NewLocation = FMath::Lerp(Node1Location, Node2Location, T) + NewRightVector * SpawnConfiguration->LaneWidth;
		
		SpawnTransforms.Push(FTransform(NewRotation, NewLocation, FVector::One()));
	}
}
