﻿// Copyright Anupam Sahu. All Rights Reserved.

#include "TrRepresentationSystem.h"

#if UE_EDITOR
#include "Editor.h"
#endif

#include "TrUtility.h"
#include "TrISMCManager.h"
#include "RpSpatialGraphComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "GameFramework/PlayerController.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "TrafficAI/Simulation/TrSimulationSystem.h"

void UTrRepresentationSystem::SpawnVehiclesOnGraph(const URpSpatialGraphComponent* NewGraphComponent, const UTrSpawnConfiguration* NewSpawnConfiguration)
{
	check(NewGraphComponent);
	check(NewSpawnConfiguration);
	check(NewSpawnConfiguration->VehicleVariants.Num() > 0);
	
	MeshPositionOffset = NewSpawnConfiguration->MeshPositionOffset;
	FTrVehicleStartCreator::CreateVehicleStartsOnGraph(NewGraphComponent, NewSpawnConfiguration, MaxInstances, VehicleStarts);

	for (const FTrVehiclePathTransform& StartData : VehicleStarts)
	{
		FTrafficAISpawnRequest NewSpawnRequest;
		NewSpawnRequest.Transform = StartData.Transform;
		
		FTrVehicleDefinition ChosenVariant = NewSpawnConfiguration->VehicleVariants[0];
		for(const FTrVehicleDefinition& Variant : NewSpawnConfiguration->VehicleVariants)
		{
			if(UKismetMathLibrary::RandomBoolWithWeight(Variant.Ratio))
			{
				ChosenVariant = Variant;
				break;
			}
		}
			
		// TODO : support for multiple definitions
		NewSpawnRequest.LOD1_Actor = ChosenVariant.ActorClass;
		NewSpawnRequest.LOD2_Mesh = ChosenVariant.StaticMesh;
		
		SpawnSingleVehicle(NewSpawnRequest);
	}
}

void UTrRepresentationSystem::SpawnSingleVehicle(const FTrafficAISpawnRequest& SpawnRequest)
{
	if(!ISMCManager)
	{
		ISMCManager = GetWorld()->SpawnActor<ATrISMCManager>();
		check(ISMCManager);
	}
	
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
		UStaticMesh* Mesh = SpawnRequest.LOD2_Mesh;
		const int32 ISMIndex = ISMCManager->AddInstance(Mesh, nullptr, SpawnRequest.Transform);
		Entities.Add({Mesh, ISMIndex, NewActor});
		if(Mesh)
		{
			const uint32 EntityIndex = Entities.Num() - 1;
			if(MeshIDs.Contains(Mesh))
			{
				MeshIDs[Mesh].Add(EntityIndex);
			}
			else
			{
				MeshIDs.Add(Mesh, {EntityIndex});
			}
		}
		SET_ACTOR_ENABLED(NewActor, false);
	}
}

void UTrRepresentationSystem::UpdateLODs()
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

	SimulationSystem->GetVehicleTransforms(VehicleTransforms, MeshPositionOffset);
	for(auto KVP : MeshIDs)
	{
		const TArray<uint32>& Indices = KVP.Value;
		TArray<FTransform> Transforms;
		for(uint32 Index : Indices)
		{
			Transforms.Push(VehicleTransforms[Index]);
		}

		ISMCManager->GetISMC(KVP.Key)->BatchUpdateInstancesTransforms(0, Transforms, true, true, true);
	}
}

void UTrRepresentationSystem::PostInitialize()
{
	SimulationSystem = GetWorld()->GetSubsystem<UTrSimulationSystem>();
	Super::PostInitialize();
}

bool UTrRepresentationSystem::ShouldCreateSubsystem(UObject* Outer) const
{
#if WITH_EDITOR
	return (GEditor && GEditor->IsPlaySessionInProgress());
#endif
	return true;
}

void UTrRepresentationSystem::BeginDestroy()
{
	Entities.Reset();
	Super::BeginDestroy();
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

	const TArray<FRpSpatialGraphNode>& Nodes = GraphComponent->GetNodes();
	TSet<TPair<uint32, uint32>> SeenEdges;

	uint32 NumNodes = static_cast<uint32>(Nodes.Num());
	for (uint32 Index = 0; Index < NumNodes; ++Index)
	{
		const TArray<uint32>& Connections = Nodes[Index].GetConnections();
		for (uint32 ConnectedIndex : Connections)
		{
			if (SeenEdges.Contains({Index, ConnectedIndex}) || SeenEdges.Contains({ConnectedIndex, Index}))
			{
				continue;
			}

			const FVector& Node1Location = Nodes[Index].GetLocation();
			const FVector& Node2Location = Nodes[ConnectedIndex].GetLocation();

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
