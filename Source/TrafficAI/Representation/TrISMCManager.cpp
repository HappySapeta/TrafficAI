// Copyright Anupam Sahu. All Rights Reserved.

#include "TrISMCManager.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Materials/MaterialInstance.h"

ATrISMCManager::ATrISMCManager()
{
	PrimaryActorTick.bCanEverTick = false;
}

int32 ATrISMCManager::AddInstance(UStaticMesh* Mesh, UMaterialInstance* Material, const FTransform& Transform)
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

void ATrISMCManager::RemoveInstance(UStaticMesh* Mesh, const int32 InstanceIndex)
{
	if(ISMCMap.Contains(Mesh))
	{
		ISMCMap[Mesh]->RemoveInstance(InstanceIndex);
	}
}

UHierarchicalInstancedStaticMeshComponent* ATrISMCManager::GetISMC(const UStaticMesh* Mesh) const
{
	if(ISMCMap.Contains(Mesh))
	{
		return ISMCMap[Mesh];
	}

	return nullptr;
}

void ATrISMCManager::SetInstanceTransform(const UStaticMesh* Mesh, const int32 InstanceIndex, const FTransform& InTransform) const
{
	if(ISMCMap.Contains(Mesh))
	{
		ISMCMap[Mesh]->UpdateInstanceTransform(InstanceIndex, InTransform, true, true, false);
	}
}

void ATrISMCManager::GetInstanceTransform(const UStaticMesh* Mesh, const int32 InstanceIndex, FTransform& OutTransform) const
{
	if(ISMCMap.Contains(Mesh))
	{
		ISMCMap[Mesh]->GetInstanceTransform(InstanceIndex, OutTransform, true);
	}
}
