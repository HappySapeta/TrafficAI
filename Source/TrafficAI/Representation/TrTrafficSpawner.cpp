// Copyright Anupam Sahu. All Rights Reserved.

#include "TrTrafficSpawner.h"
#include "TrRepresentationSystem.h"
#include "RpSpatialGraphComponent.h"

void UTrTrafficSpawner::SetTrafficGraph(const URpSpatialGraphComponent* NewGraphComponent)
{
	GraphComponent = NewGraphComponent;
}

void UTrTrafficSpawner::SetSpawnData(TSubclassOf<AActor> ActorClass, UStaticMesh* Mesh)
{
	DebugActor = ActorClass;
	DebugMesh = Mesh;
}

void UTrTrafficSpawner::SpawnTraffic()
{
	RepresentationSystem = GetWorld()->GetSubsystem<UTrRepresentationSystem>();
	if(IsValid(RepresentationSystem) && IsValid(GraphComponent))
	{
		TraverseGraph();
	}
}

void UTrTrafficSpawner::TraverseGraph()
{
	const TArray<FRpSpatialGraphNode>* Nodes = GraphComponent->GetNodes();
	TSet<uint32> EdgeSet;

	uint32 NumNodes = static_cast<uint32>(Nodes->Num());
	for(uint32 Index = 0; Index < NumNodes; ++Index)
	{
		TSet<uint32> ConnectedIndices = Nodes->operator[](Index).GetConnections();
		for(uint32 ConnectedIndex : ConnectedIndices)
		{
			if(EdgeSet.Contains(Index ^ ConnectedIndex))
			{
				continue;
			}

			CreateSpawnPointsBetweenNodes(Nodes->operator[](Index).GetLocation(), Nodes->operator[](ConnectedIndex).GetLocation());

			EdgeSet.Add(Index ^ ConnectedIndex);
		}
	}
}

void UTrTrafficSpawner::CreateSpawnPointsBetweenNodes(const FVector& Node1Location, const FVector& Node2Location)
{
	constexpr float AverageVehicleLength = 600.0f;
	constexpr float VariableSeparation = 0.125f;
	const float EdgeLength = FVector::Distance(Node1Location, Node2Location);
	const float RequiredSeparation = (AverageVehicleLength) / EdgeLength;

	float TMin = 0.0f;
	while(TMin < 1.0f)
	{
		float TMax = TMin + VariableSeparation;
		float T = FMath::RandRange(TMin, TMax);
		TMin = T + RequiredSeparation;
		
		const FVector NewLocation = FMath::Lerp(Node1Location, Node2Location, T); 
		
		DrawDebugPoint(GetWorld(), NewLocation, 20.0f, FColor::MakeRandomColor(), true);
        
        FTrafficAISpawnRequest SpawnRequest;
        SpawnRequest.Transform = FTransform(NewLocation);
        SpawnRequest.LOD1_Actor = DebugActor;
        SpawnRequest.LOD2_Mesh = DebugMesh;
        RepresentationSystem->SpawnDeferred(SpawnRequest);
	}
}
