// Copyright Anupam Sahu. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "TrafficAIRepresentationSystem.generated.h"

USTRUCT(BlueprintType)
struct FTrafficRepresentation
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UStaticMesh> StaticMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<AActor> Dummy;

	// Final proportion values are normalized.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (UIMin = 0.0, UIMax = 1.0, ClampMin = 0.0, ClampMax = 0.0))
	float Proportion;
};

/**
 * 
 */
UCLASS()
class TRAFFICAI_API ATrafficAIRepresentationSystem : public AActor
{
	GENERATED_BODY()

public:
	
	ATrafficAIRepresentationSystem();

	UFUNCTION(BlueprintCallable, CallInEditor)
	void DebugGenerateRandomInstances();

	UFUNCTION(BlueprintCallable, CallInEditor)
	void DebugClearInstances();
	
	void GenerateInstances(FTrafficRepresentation Distribution, const TArray<FTransform>& Transforms, int MaxCount);

protected:
	
	virtual void BeginPlay() override;

public:
	
	virtual void Tick(float DeltaSeconds) override;

private:
	UPROPERTY()
	TArray<AActor*> Dummies;

	UPROPERTY()
	UInstancedStaticMeshComponent* ISMC;

	UPROPERTY(EditAnywhere)
	FTrafficRepresentation SampleDistribution;
	
	UPROPERTY(Transient)
	TArray<AActor*> ActorInstances;

	TArray<FTransform> InstanceTransforms;

	UPROPERTY()
	AActor* LocalPlayer;

	UPROPERTY(EditAnywhere)
	TObjectPtr<UMaterialInstance> ISMC_Material;

	// Actor Range
	UPROPERTY(EditAnywhere)
	FFloatRange L1_Range;

	// ISMC Range
	UPROPERTY(EditAnywhere)
	FFloatRange L0_Range;

	UPROPERTY(EditAnywhere)
	int NumberOfInstances;
	
};
