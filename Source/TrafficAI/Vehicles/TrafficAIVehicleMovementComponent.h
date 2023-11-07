// Copyright Anupam Sahu. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "TrafficAIVehicleMovementComponent.generated.h"


class TRAFFICAI_API UTrafficAIVehicleSimulation : public UChaosWheeledVehicleSimulation
{
public:
	
	void SetSimulationEnabled(const bool bInEnable);

	virtual void ApplyInput(const FControlInputs& ControlInputs, float DeltaTime) override;
	virtual void ProcessMechanicalSimulation(float DeltaTime) override;
	virtual void ProcessSteering(const FControlInputs& ControlInputs) override;

private:

	bool bIsSimEnabled = true;
	
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class TRAFFICAI_API UTrafficAIVehicleMovementComponent : public UChaosWheeledVehicleMovementComponent
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable)
	void SetSimulationEnabled(const bool bInEnable);
	
	virtual TUniquePtr<Chaos::FSimpleWheeledVehicle> CreatePhysicsVehicle() override;
};
