// Copyright Anupam Sahu. All Rights Reserved.

#include "TrafficAIVisualizer.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Materials/MaterialInstance.h"

ATrafficAIVisualizer::ATrafficAIVisualizer()
{
	PrimaryActorTick.bCanEverTick = false;
}

int32 ATrafficAIVisualizer::AddInstance(UStaticMesh* Mesh, UMaterialInstance* Material, const FTransform& Transform)
{
	if(!ISMCMap.Contains(Mesh))
	{
		UHierarchicalInstancedStaticMeshComponent* NewISMC = Cast<UHierarchicalInstancedStaticMeshComponent>(AddComponentByClass(UHierarchicalInstancedStaticMeshComponent::StaticClass(), false, FTransform::Identity, false));
		NewISMC->SetStaticMesh(Mesh);
		if(IsValid(Material))
		{
			NewISMC->SetMaterial(0, Material);
		}
		ISMCMap.Add(Mesh, NewISMC);
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

UHierarchicalInstancedStaticMeshComponent* ATrafficAIVisualizer::GetISMC(const UStaticMesh* Mesh) const
{
	if(ISMCMap.Contains(Mesh))
	{
		return ISMCMap[Mesh];
	}

	return nullptr;
}
