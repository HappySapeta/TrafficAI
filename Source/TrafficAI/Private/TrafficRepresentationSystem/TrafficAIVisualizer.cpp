// Copyright Anupam Sahu. All Rights Reserved.

#include "TrafficRepresentationSystem/TrafficAIVisualizer.h"
#include "Components/InstancedStaticMeshComponent.h"

ATrafficAIVisualizer::ATrafficAIVisualizer()
{
	PrimaryActorTick.bCanEverTick = false;
}

int32 ATrafficAIVisualizer::AddInstance(UStaticMesh* Mesh, const FTransform& Transform)
{
	if(!ISMCMap.Contains(Mesh))
	{
		ISMCMap.Add(Mesh, NewObject<UInstancedStaticMeshComponent>(this));
	}
	
	return ISMCMap[Mesh]->AddInstance(Transform, true);
}

void ATrafficAIVisualizer::RemoveInstance(UStaticMesh* Mesh, const int32 InstanceIndex)
{
	if(ISMCMap.Contains(Mesh))
	{
		ISMCMap[Mesh]->RemoveInstance(InstanceIndex);
	}
}

const UInstancedStaticMeshComponent* ATrafficAIVisualizer::GetISMC(const UStaticMesh* Mesh) const
{
	if(ISMCMap.Contains(Mesh))
	{
		return ISMCMap[Mesh];
	}

	return nullptr;
}
