// Copyright Anupam Sahu. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "TrVehicle.generated.h"

/**
 *
 */
UCLASS()
class TRAFFICAI_API ATrVehicle : public APawn
{
	GENERATED_BODY()

public:
	
	// Sets default values for this pawn's properties
	ATrVehicle();

	UFUNCTION(BlueprintCallable)
	UPrimitiveComponent* GetRoot() const { return VehicleRoot; }
	
protected:

	// The skeletal mesh component representing the vehicle.
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Vehicle")
	TObjectPtr<USkeletalMeshComponent> VehicleRoot;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vehicle")
	TObjectPtr<class UTrVehicleMovementComponent> VehicleMovementComponent;
	
};
