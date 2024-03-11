// Copyright Anupam Sahu. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "WheeledVehiclePawn.h"
#include "PIDController/FRpPIDController.h"
#include "TrVehicle.generated.h"

/**
 *
 */
UCLASS()
class TRAFFICAI_API ATrVehicle : public AWheeledVehiclePawn
{
	GENERATED_BODY()

public:

	ATrVehicle();

	virtual void Tick(float DeltaSeconds) override;
	
	void SetAcceleration(const float DesiredAcceleration);

	void SetHeading(const FVector& DesiredHeading);

private:

	FVector PreviousVelocity = FVector::ZeroVector;
	float Acceleration = 0.0f;
	float DesiredAcceleration = 0.0f;
	FRpPIDController<float> AccelerationController;
	
};
