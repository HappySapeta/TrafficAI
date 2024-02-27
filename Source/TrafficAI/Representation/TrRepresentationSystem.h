// Copyright Anupam Sahu. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "..\Shared\TrTypes.h"
#include "UObject/WeakFieldPtr.h"
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
	
	// Traverse the edges of the Graph
	static void CreateVehicleStartsOnGraph
	(
		const URpSpatialGraphComponent* GraphComponent,
		const UTrSpawnConfiguration* SpawnConfiguration,
		const int MaxInstances,
		TArray<FTrVehiclePathTransform>& OutVehicleStarts
	);

	// Create spawn points along an edge
	static void CreateStartTransformsOnEdge
	(
		const FVector& Start,
		const FVector& Destination,
		const UTrSpawnConfiguration* SpawnConfiguration,
		TArray<FTrVehiclePathTransform>& OutStartData
	);
};

/**
 * Subsystem responsible for spawning Entities and handling the seamless transition of LODs.
 */
UCLASS(config = Game, DefaultConfig, DisplayName = "Traffic Representation System")
class TRAFFICAI_API UTrRepresentationSystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	
	// Spawn Vehicles
	UFUNCTION(BlueprintCallable)
	void SpawnOnGraph(const URpSpatialGraphComponent* NewGraphComponent, const UTrSpawnConfiguration* NewRequestData);

	// Push a request to spawn an Entity. The request is not guaranteed to be processed immediately.
	UFUNCTION(BlueprintCallable)
	void SpawnSingleVehicle(const FTrafficAISpawnRequest& SpawnRequest);

	// Returns a const reference to an array of Entities.
	const TArray<FTrVehicleRepresentation>& GetEntities() const { return Entities; }

	// Returns a const reference to an array of Vehicle Start Transforms.
	const TArray<FTrVehiclePathTransform>& GetVehicleStarts() const { return VehicleStarts; }
	
	void StartSimulation() {};

	virtual void PostInitialize() override;

	// Reset SharedPtrs to Entities.
	virtual void BeginDestroy() override;

	// Create this Subsystem only if playing in PIE or in game.
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

protected:
	
	// Spawn an ISMCManager and set timers.
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	// Clear all timers.
	virtual void Deinitialize() override;

private:
	
	void InitializeLODUpdater();

protected:
	
	TArray<FTrVehicleRepresentation> Entities;

	UPROPERTY()
	TObjectPtr<class ATrISMCManager> ISMCManager;

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
	
private:

	FTimerHandle MainTimer;
	TArray<FTrafficAISpawnRequest> SpawnRequests;
	TArray<FTrVehiclePathTransform> VehicleStarts;
};
