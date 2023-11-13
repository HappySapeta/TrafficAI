// Copyright Anupam Sahu. All Rights Reserved.

#pragma once

#define SET_ACTOR_ENABLED(Actor, Value) Actor->SetActorEnableCollision(Value); Actor->SetActorHiddenInGame(!Value); Actor->SetActorTickEnabled(Value);

enum class ELODLevel : int8
{
	None = -1,
	LOD0 = 0,
	LOD1 = 1
};

// Simulated Entity
struct FTrEntity
{
	// Mesh used for the lowest LOD.
	UStaticMesh* Mesh = nullptr;
	
	// Index of the Instanced Static Mesh associated with this Entity.
	// The InstanceIndex combined with the Mesh reference can be used to uniquely identify this Entity.
	int32 InstanceIndex = -1;

	// Actor used for the highest LOD.
	AActor* Dummy = nullptr;

	ELODLevel LODLevel;

	ELODLevel PreviousLODLevel = ELODLevel::None;
};