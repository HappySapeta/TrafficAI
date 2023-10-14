// Copyright Anupam Sahu. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "TrafficAIVehicle.generated.h"

USTRUCT(Blueprintable, BlueprintType)
struct TRAFFICAI_API FComponentWrapper
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere)
	class USphereComponent* SphereComponent;
};

UCLASS()
class TRAFFICAI_API ATrafficAIVehicle : public APawn
{
	GENERATED_BODY()

public:
	
	// Sets default values for this pawn's properties
	ATrafficAIVehicle();
	
	void SetupWheel(const char* Suffix);

protected:

	UPROPERTY(VisibleAnywhere, Category = "Vehicle")
	TObjectPtr<USkeletalMeshComponent> VehicleMesh;
	
	UPROPERTY(VisibleAnywhere)
	TArray<class USphereComponent*> SphereColliders;

	UPROPERTY(EditAnywhere)
	TArray<class UPhysicsConstraintComponent*> SuspensionConstraints;
	
	UPROPERTY(EditAnywhere)
	TArray<class UPhysicsConstraintComponent*> AxisConstraints;	
};
