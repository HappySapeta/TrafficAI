// Copyright Anupam Sahu. All Rights Reserved.

#include "TrTrafficSpawner.h"
#include "TrRepresentationSystem.h"
#include "RpSpatialGraphComponent.h"
#include "Kismet/KismetMathLibrary.h"

void UTrTrafficSpawner::SetTrafficGraph(const URpSpatialGraphComponent* NewGraphComponent)
{
	GraphComponent = NewGraphComponent;
}

void UTrTrafficSpawner::SetSpawnData(const TSubclassOf<AActor>& ActorClass, UStaticMesh* Mesh)
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
	TArray<const URpSpatialGraphNode*> Nodes = GraphComponent->GetNodes();
	TSet<TPair<const URpSpatialGraphNode*, const URpSpatialGraphNode*>> EdgeSet;

	for(const URpSpatialGraphNode* Node : Nodes)
	{
		TSet<const URpSpatialGraphNode*> Connections = Node->GetConnections();
		for(const URpSpatialGraphNode* Connection : Connections)
		{
			if(EdgeSet.Contains({Node, Connection}) || EdgeSet.Contains({Connection, Node}))
			{
				continue;
			}

			CreateSpawnPointsBetweenNodes(Node, Connection);

			EdgeSet.Add({Node, Connection});
			EdgeSet.Add({Connection, Node});
		}
	}
}

void UTrTrafficSpawner::CreateSpawnPointsBetweenNodes(const URpSpatialGraphNode* Node1, const URpSpatialGraphNode* Node2)
{
	constexpr float AverageVehicleLength = 200.0f;
	constexpr float MinimumGap = 100.0f;
	constexpr float PackingFactor = 0.5f;

	const FVector& P1 = Node1->GetLocation();
	const FVector& P2 = Node2->GetLocation();
	const float EdgeLength = FVector::Distance(P1, P2);
	const float Separation = (AverageVehicleLength * 0.5f + MinimumGap) / EdgeLength;

	float LowerLimit = 0.1f;
	while(LowerLimit < 0.9f)
	{
		float UpperLimit = FMath::Min(LowerLimit + (AverageVehicleLength * 0.5f) / EdgeLength + PackingFactor, 0.9f);
		float Alpha = FMath::RandRange(LowerLimit, UpperLimit);
		const FVector& NewLocation = FMath::Lerp(P1, P2, Alpha);
		
		DrawDebugPoint(GetWorld(), NewLocation, 5.0f, FColor::Blue, true);

		FTrafficAISpawnRequest SpawnRequest;
		SpawnRequest.Transform = FTransform(NewLocation);
		SpawnRequest.LOD1_Actor = DebugActor;
		SpawnRequest.LOD2_Mesh = DebugMesh;
		RepresentationSystem->SpawnDeferred(SpawnRequest);

		LowerLimit = Alpha + Separation;
	}
}
