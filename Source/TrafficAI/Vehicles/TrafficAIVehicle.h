// Copyright Anupam Sahu. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "TrafficAIVehicle.generated.h"

/**
 *
 */
UCLASS()
class TRAFFICAI_API ATrafficAIVehicle : public APawn
{
	GENERATED_BODY()

public:
	
	// Sets default values for this pawn's properties
	ATrafficAIVehicle();

	UFUNCTION(BlueprintCallable)
	UPrimitiveComponent* GetRoot() const { return VehicleRoot; }

	UFUNCTION(BlueprintCallable)
	void SetComplexSimulationEnabled(const bool bInEnable);

protected:

	// The skeletal mesh component representing the vehicle.
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Vehicle")
	TObjectPtr<USkeletalMeshComponent> VehicleRoot;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vehicle")
	TObjectPtr<class UTrVehicleMovementComponent> VehicleMovementComponent;

private:

	bool bIsComplexSimulationEnabled;
	
};
