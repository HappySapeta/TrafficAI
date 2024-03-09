// Copyright Anupam Sahu. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "TrTypes.h"
#include "TrRepresentationSystem.generated.h"

// Information required to spawn an Entity.
USTRUCT(BlueprintType)
struct TRAFFICAI_API FTrafficAISpawnRequest
{
	GENERATED_BODY()

	// Static Mesh used by the Instanced Static Mesh Renderer.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UStaticMesh* LOD2_Mesh;

	// An Actor that represents an Active vehicle with maximum details.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<AActor> LOD1_Actor;

	// Initial transform when the Entity is spawned.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FTransform Transform;
};

class TRAFFICAI_API FTrVehicleStartCreator
{
public:
	
	/**
	 * \brief Creates vehicle start positions along the specified graph.
	 *
	 * This method takes a graph component, spawn configuration, maximum number of instances, and an array of vehicle start positions.
	 * It iterates through all nodes in the graph and creates start positions between connected nodes.
	 * When creating a start position, it makes use of the spawn configuration to determine the number of instances and other properties
	 */
	static void CreateVehicleStartsOnGraph
	(
		const URpSpatialGraphComponent* GraphComponent,
		const UTrSpawnConfiguration* SpawnConfiguration,
		const int MaxInstances,
		TArray<FTrVehiclePathTransform>& OutVehicleStarts
	);

	/**
	 * \brief Creates vehicle start positions along an edge between specified start and destination points.
	 *
	 * This method takes the start position, destination position, spawn configuration, and an array of vehicle start positions.
	 * It creates start positions along the edge between the start and destination points based on the spawn configuration.
	 * The created start positions are added to the OutStartData array.
	 */
	static void CreateStartTransformsOnEdge
	(
		const FVector& Start,
		const FVector& Destination,
		const UTrSpawnConfiguration* SpawnConfiguration,
		TArray<FTrVehiclePathTransform>& OutStartData
	);
};

/**
 * \brief The UTrRepresentationSystem class is responsible for managing the representation of vehicles in the game world.
 *
 * The UTrRepresentationSystem class handles the spawning and updating of vehicle representations in the game world.
 * It provides methods to spawn vehicles on a specified graph and to push requests to spawn single vehicles.
 * It also provides methods to retrieve references to the spawned entities and the vehicle start transforms.
 * The UTrRepresentationSystem class is a part of the Traffic AI system in the game.
 */
UCLASS(config = Game, DefaultConfig, DisplayName = "Traffic Representation System")
class TRAFFICAI_API UTrRepresentationSystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	
	// Spawn Vehicles
	UFUNCTION(BlueprintCallable)
	void SpawnVehiclesOnGraph(const URpSpatialGraphComponent* NewGraphComponent, const UTrSpawnConfiguration* NewRequestData);

	// Push a request to spawn an Entity. The request is not guaranteed to be processed immediately.
	UFUNCTION(BlueprintCallable)
	void SpawnSingleVehicle(const FTrafficAISpawnRequest& SpawnRequest);
	
	// Returns a const reference to an array of Vehicle Start Transforms.
	const TArray<FTrVehiclePathTransform>& GetVehicleStarts() const { return VehicleStarts; }

	const TArray<FTransform>& GetInitialTransforms() const;
	
	// Reset SharedPtrs to Entities.
	virtual void BeginDestroy() override;

	// Create this Subsystem only if playing in PIE or in game.
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

	//This method iterates through the entities in the system and switches their LODs based on the distance to the player. 
	void UpdateLODs();
	
	virtual void PostInitialize() override;

protected:
	
	UPROPERTY()
	TObjectPtr<class ATrISMCManager> ISMCManager;

	uint32 NumEntities;

private:

	// Maximum number of Entities that can be spawned.
	UPROPERTY(Config, EditAnywhere, Category = "Representation System | Spawn Settings", meta = (TitleProperty = "Maximum number of Entities", ClampMin = 0, UIMin = 0))
	int MaxInstances = 1000;
	
	// Amount of Entities updated in a single batch.
	UPROPERTY(Config, EditAnywhere, Category = "Representation System | Update Settings", meta = (TitleProperty = "Entity Spawn & Update Batch Size", ClampMin = 1, UIMin = 1))
	uint8 ProcessingBatchSize = 100;

	// The range in which Actors become relevant. Since Actors have physics simulations, they are more expensive to simulate.
	UPROPERTY(Config, EditAnywhere, Category = "Representation System | Update Settings")
	FFloatRange ActorRelevancyRange;

	// The range within which Static Mesh Instances replace Actors.
	UPROPERTY(Config, EditAnywhere, Category = "Representation System | Update Settings")
	FFloatRange StaticMeshRelevancyRange;

	UPROPERTY()
	TObjectPtr<class UTrSimulationSystem> SimulationSystem; 
	
private:

	TArray<AActor*> Actors;
	TMap<UStaticMesh*, TArray<uint32>> MeshIDs;
	
	FVector MeshPositionOffset;
	TArray<FTransform> VehicleTransforms;

	TArray<FTrafficAISpawnRequest> SpawnRequests;
	TArray<FTrVehiclePathTransform> VehicleStarts;
};
