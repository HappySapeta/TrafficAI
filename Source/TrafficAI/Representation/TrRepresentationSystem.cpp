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

void UTrRepresentationSystem::SpawnOnGraph(const URpSpatialGraphComponent* NewGraphComponent, const UTrSpawnConfiguration* NewSpawnConfiguration)
{
	if (IsValid(NewGraphComponent))
	{
		FTrVehicleStartCreator::CreateVehicleStartsOnGraph(NewGraphComponent, NewSpawnConfiguration, MaxInstances, VehicleStarts);

		for (const FTrVehiclePathTransform& StartData : VehicleStarts)
		{
			FTrafficAISpawnRequest NewSpawnRequest;
			NewSpawnRequest.Transform = StartData.Transform;

			// TODO : support for multiple definitions
			NewSpawnRequest.LOD1_Actor = NewSpawnConfiguration->TrafficDefinitions[0].ActorClass;
			NewSpawnRequest.LOD2_Mesh = NewSpawnConfiguration->TrafficDefinitions[0].StaticMesh;

			SpawnSingleVehicle(NewSpawnRequest);
		}
	}
}

void UTrRepresentationSystem::SpawnSingleVehicle(const FTrafficAISpawnRequest& SpawnRequest)
{
	URpDeferredBatchProcessingSystem* BatchProcessor = GetWorld()->GetSubsystem<URpDeferredBatchProcessingSystem>();
	if (ensure(BatchProcessor))
	{
		BatchProcessor->QueueCommand("SpawnProcessor", [this, SpawnRequest]()
		{
			if(Entities.Num() >= MaxInstances)
			{
				return;
			}
			
			static FActorSpawnParameters SpawnParameters;
#if UE_EDITOR
			SpawnParameters.bHideFromSceneOutliner = true;
#endif
			if (AActor* NewActor = GetWorld()->SpawnActor(SpawnRequest.LOD1_Actor, &SpawnRequest.Transform, SpawnParameters))
			{
				checkf(ISMCManager, TEXT("[UTrRepresentationSystem][ProcessSpawnRequests] Reference to the ISMCManager is null."))
				const int32 ISMIndex = ISMCManager->AddInstance(SpawnRequest.LOD2_Mesh, nullptr, SpawnRequest.Transform);
				Entities.Add({SpawnRequest.LOD2_Mesh, ISMIndex, NewActor});
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
	if (ensureMsgf(BatchProcessor, TEXT("BatchProcessor not found.")))
	{
		BatchProcessor->QueueCommand("LODUpdateProcessor", [this]()
		{
			TRACE_CPUPROFILER_EVENT_SCOPE(UTrRepresentationSystem::UpdateLODLambda)

			static int EntityIndex = 0;
			if (EntityIndex >= Entities.Num())
			{
				EntityIndex = 0;
			}

			FVector FocusLocation(0.0f);
			if(APawn* Pawn = GetWorld()->GetFirstPlayerController()->GetPawn())
			{
				FocusLocation = Pawn->GetActorLocation();	
			}

			int CurrentBatchSize = 0;
			while (EntityIndex < Entities.Num() && CurrentBatchSize < ProcessingBatchSize)
			{
				const FTrVehicleRepresentation& Entity = Entities.operator[](EntityIndex);
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

void FTrVehicleStartCreator::CreateVehicleStartsOnGraph
(
	const URpSpatialGraphComponent* GraphComponent,
	const UTrSpawnConfiguration* SpawnConfiguration,
	const int MaxInstances,
	TArray<FTrVehiclePathTransform>& OutVehicleStarts
)
{
	check(SpawnConfiguration);

	const TArray<FRpSpatialGraphNode>* Nodes = GraphComponent->GetNodes();
	TSet<TPair<uint32, uint32>> SeenEdges;

	uint32 NumNodes = static_cast<uint32>(Nodes->Num());
	for (uint32 Index = 0; Index < NumNodes; ++Index)
	{
		const TArray<uint32>& Connections = Nodes->operator[](Index).GetConnections();
		for (uint32 ConnectedIndex : Connections)
		{
			if (SeenEdges.Contains({Index, ConnectedIndex}) || SeenEdges.Contains({ConnectedIndex, Index}))
			{
				continue;
			}

			const FVector& Node1Location = Nodes->operator[](Index).GetLocation();
			const FVector& Node2Location = Nodes->operator[](ConnectedIndex).GetLocation();

			auto PushVehicleStartsLambda = [SpawnConfiguration, MaxInstances, &OutVehicleStarts]
			(
				const FVector& FirstLocation,
				const FVector& SecondLocation,
				const uint32 FirstIndex,
				const uint32 SecondIndex
			)
			{
				TArray<FTrVehiclePathTransform> TempVehicleStarts;
				CreateStartTransformsOnEdge(FirstLocation, SecondLocation, SpawnConfiguration, TempVehicleStarts);
				for(FTrVehiclePathTransform& StartData : TempVehicleStarts)
				{
					if(OutVehicleStarts.Num() >= MaxInstances)
					{
						return;
					}
				
					StartData.Path.StartNodeIndex = FirstIndex;
					StartData.Path.EndNodeIndex = SecondIndex;
					StartData.Path.Start = FirstLocation;
					StartData.Path.End = SecondLocation;

					OutVehicleStarts.Push(StartData);
				}
			};

			PushVehicleStartsLambda(Node1Location, Node2Location, Index, ConnectedIndex);
			PushVehicleStartsLambda(Node2Location, Node1Location, ConnectedIndex, Index);
			
			SeenEdges.Add({Index, ConnectedIndex});
			SeenEdges.Add({ConnectedIndex, Index});
		}
	}
}

void FTrVehicleStartCreator::CreateStartTransformsOnEdge
(
	const FVector& Start,
	const FVector& Destination,
	const UTrSpawnConfiguration* SpawnConfiguration,
	TArray<FTrVehiclePathTransform>& OutStartData
)
{
	const FVector NewForwardVector = (Destination - Start).GetSafeNormal();
	const FRotator NewRotation = UKismetMathLibrary::MakeRotFromX(NewForwardVector);
	const FVector NewRightVector = NewForwardVector.Cross(FVector::UpVector);
	
	const float EdgeLength = FVector::Distance(Start, Destination);
	const float NormMinSeparation = SpawnConfiguration->Separation.GetLowerBoundValue() /  EdgeLength;
	const float NormMaxSeparation = SpawnConfiguration->Separation.GetUpperBoundValue() / EdgeLength;
	const float NormCutOff = SpawnConfiguration->IntersectionCutoff / EdgeLength;
	float Alpha = NormCutOff;
	const float MaxAlpha = 1.0f - NormCutOff;
	while (Alpha < MaxAlpha)
	{
		Alpha = Alpha + FMath::RandRange(NormMinSeparation, NormMaxSeparation);
		if(Alpha >= MaxAlpha)
		{
			break;
		}
		
		const FVector NewLocation = FMath::Lerp(Start, Destination, Alpha) + NewRightVector * SpawnConfiguration->LaneWidth;
		OutStartData.Push({FTransform(NewRotation, NewLocation, FVector::One()), FTrPath()});
	}
}
