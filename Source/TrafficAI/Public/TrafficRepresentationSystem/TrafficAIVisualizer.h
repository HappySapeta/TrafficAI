// Copyright Anupam Sahu. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TrafficAIVisualizer.generated.h"

UCLASS(NotPlaceable, Transient, NotBlueprintable)
class TRAFFICAI_API ATrafficAIVisualizer : public AActor
{
	GENERATED_BODY()

	friend class UTrafficAIRepresentationSystem;

public:
	
	// Sets default values for this actor's properties
	ATrafficAIVisualizer();
	
private:
	
	/**
	 * Add an instance of Mesh to an Instanced Static Mesh Renderer.
	 * @return Index of the new Instance.
	 */
	int32 AddInstance(UStaticMesh* Mesh, UMaterialInstance* Material = nullptr, const FTransform& Transform = FTransform::Identity);

	void RemoveInstance(UStaticMesh* Mesh, const int32 InstanceIndex);

	// Get an InstancedStaticMeshComponent that renders a specific Mesh. 
	UHierarchicalInstancedStaticMeshComponent* GetISMC(const UStaticMesh* Mesh) const;

private:

	UPROPERTY()
	TMap<UStaticMesh*, UHierarchicalInstancedStaticMeshComponent*> ISMCMap;
};
