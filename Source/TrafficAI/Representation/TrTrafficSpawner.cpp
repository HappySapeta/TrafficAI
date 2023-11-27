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
		// 2. Call RepresentationSystem->SpawnDeferred for each spawn point.
		// 3. Call it a day.
	}
}

void UTrTrafficSpawner::TraverseGraph()
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

			CreateSpawnPointsBetweenNodes(Nodes->operator[](Index).GetLocation(), Nodes->operator[](ConnectedIndex).GetLocation());
		}
	}
}

void UTrTrafficSpawner::CreateSpawnPointsBetweenNodes(const FVector& Node1Location, const FVector& Node2Location)
{
	constexpr float AverageVehicleLength = 600.0f;
	constexpr float MinimumGap = 200.0f;
	constexpr float PackingFactor = 0.9f;

	const float EdgeLength = FVector::Distance(Node1Location, Node2Location);
	const float Separation = (AverageVehicleLength * 0.5f + MinimumGap) / EdgeLength;

	float LowerLimit = 0.1f;
	while(LowerLimit < 0.9f)
	{
		float UpperLimit = FMath::Min(LowerLimit + (AverageVehicleLength * 0.5f) / EdgeLength + PackingFactor, 0.9f);
		float Alpha = FMath::RandRange(LowerLimit, UpperLimit);
		const FVector& NewLocation = FMath::Lerp(Node1Location, Node2Location, Alpha);
		
		DrawDebugPoint(GetWorld(), NewLocation, 5.0f, FColor::Blue, true);

		FTrafficAISpawnRequest SpawnRequest;
		SpawnRequest.Transform = FTransform(NewLocation);
		SpawnRequest.LOD1_Actor = DebugActor;
		SpawnRequest.LOD2_Mesh = DebugMesh;
		RepresentationSystem->SpawnDeferred(SpawnRequest);

		LowerLimit = Alpha + Separation;
	}
}
