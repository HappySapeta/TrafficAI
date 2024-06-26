﻿// Copyright Anupam Sahu. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TrISMCManager.generated.h"

UCLASS(NotPlaceable, Transient, NotBlueprintable)
class TRAFFICAI_API ATrISMCManager : public AActor
{
	GENERATED_BODY()

	friend class UTrRepresentationSystem;

public:
	
	// Sets default values for this actor's properties
	ATrISMCManager();

	void SetInstanceTransform(const UStaticMesh* Mesh, const int32 InstanceIndex, const FTransform& InTransform) const;

	void GetInstanceTransform(const UStaticMesh* Mesh, const int32 InstanceIndex, FTransform& OutTransform) const;

private:

	// Get an InstancedStaticMeshComponent that renders a specific Mesh. 
	UHierarchicalInstancedStaticMeshComponent* GetISMC(const UStaticMesh* Mesh) const;

	/**
	 * Add an instance of Mesh to an Instanced Static Mesh Renderer.
	 * @return Index of the new Instance.
	 */
	int32 AddInstance(UStaticMesh* Mesh, UMaterialInstance* Material = nullptr, const FTransform& Transform = FTransform::Identity);

	void RemoveInstance(UStaticMesh* Mesh, const int32 InstanceIndex);

private:

	UPROPERTY()
	TMap<UStaticMesh*, UHierarchicalInstancedStaticMeshComponent*> ISMCMap;
};
