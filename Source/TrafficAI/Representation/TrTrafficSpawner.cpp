// Copyright Anupam Sahu. All Rights Reserved.

#include "TrTrafficSpawner.h"
#include "TrRepresentationSystem.h"
#include "RpSpatialGraphComponent.h"
#include "Kismet/KismetMathLibrary.h"

void UTrTrafficSpawner::Spawn(const URpSpatialGraphComponent* NewGraphComponent, const UTrTrafficSpawnConfiguration* NewRequestData)
{
	UTrRepresentationSystem* RepresentationSystem = GetWorld()->GetSubsystem<UTrRepresentationSystem>();
	SpawnRequestData = NewRequestData;
	
	if(IsValid(RepresentationSystem) && IsValid(NewGraphComponent))
	{
		TArray<TArray<FTransform>> GraphPoints;
		CreateSpawnPointsOnGraph(NewGraphComponent, GraphPoints);
		
		for(const TArray<FTransform>& Edge : GraphPoints)
		{
			for(const FTransform& SpawnTransform : Edge)
			{
				FTrafficAISpawnRequest NewSpawnRequest;
				NewSpawnRequest.Transform = SpawnTransform;
				NewSpawnRequest.LOD1_Actor = NewRequestData->TrafficDefinitions[0].ActorClass;
				NewSpawnRequest.LOD2_Mesh = NewRequestData->TrafficDefinitions[0].StaticMesh;
				
				RepresentationSystem->SpawnDeferred(NewSpawnRequest);
			}
		}
	}
}

void UTrTrafficSpawner::CreateSpawnPointsOnGraph(const URpSpatialGraphComponent* GraphComponent, TArray<TArray<FTransform>>& GraphSpawnPoints)
{
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
			
			CreateSpawnPointsOnEdge(Node1Location, Node2Location, SpawnPoints);
			CreateSpawnPointsOnEdge(Node2Location, Node1Location, SpawnPoints);
			GraphSpawnPoints.Push(SpawnPoints);
			
			EdgeSet.Add({Index, ConnectedIndex});
			EdgeSet.Add({ConnectedIndex, Index});
		}
	}
}

void UTrTrafficSpawner::CreateSpawnPointsOnEdge(const FVector& Node1Location, const FVector& Node2Location, TArray<FTransform>& SpawnTransforms)
{
	const float EdgeLength = FVector::Distance(Node1Location, Node2Location);
	const float NormalizedMinimumSeparation = (SpawnRequestData->MinimumSeparation) / EdgeLength;
	
	float TMin = SpawnRequestData->IntersectionCutoff;
	while(TMin < 1.0f - SpawnRequestData->IntersectionCutoff)
	{
		float TMax = TMin + (1 - FMath::Clamp(SpawnRequestData->VariableSeparation, 0.0f, 1.0f));
		float T = FMath::Clamp(FMath::RandRange(TMin, TMax), 0.0f, 1.0f);
		
		TMin = T + NormalizedMinimumSeparation;
		
		const FVector NewForwardVector = (Node2Location - Node1Location).GetSafeNormal();
		const FRotator NewRotation = UKismetMathLibrary::MakeRotFromX(NewForwardVector);
		const FVector NewRightVector = NewForwardVector.Cross(FVector::UpVector);
		const FVector NewLocation = FMath::Lerp(Node1Location, Node2Location, T) + NewRightVector * SpawnRequestData->LaneWidth;
		
		SpawnTransforms.Push(FTransform(NewRotation, NewLocation, FVector::One()));
	}
}
