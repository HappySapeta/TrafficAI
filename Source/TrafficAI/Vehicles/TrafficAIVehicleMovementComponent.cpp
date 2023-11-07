// Copyright Anupam Sahu. All Rights Reserved.

#include "TrafficAIVehicleMovementComponent.h"

void UTrafficAIVehicleSimulation::SetSimulationEnabled(const bool bInEnable)
{
	bIsSimEnabled = bInEnable;
}

void UTrafficAIVehicleSimulation::ApplyInput(const FControlInputs& ControlInputs, float DeltaTime)
{
	if(!bIsSimEnabled)
	{
		return;
	}
	UChaosWheeledVehicleSimulation::ApplyInput(ControlInputs, DeltaTime);
}

void UTrafficAIVehicleSimulation::ProcessMechanicalSimulation(float DeltaTime)
{
	if(!bIsSimEnabled)
	{
		return;
	}
	UChaosWheeledVehicleSimulation::ProcessMechanicalSimulation(DeltaTime);
}

void UTrafficAIVehicleSimulation::ProcessSteering(const FControlInputs& ControlInputs)
{
	if(!bIsSimEnabled)
	{
		return;
	}
	UChaosWheeledVehicleSimulation::ProcessSteering(ControlInputs);
}

void UTrafficAIVehicleMovementComponent::SetSimulationEnabled(const bool bInEnable)
{
	static_cast<UTrafficAIVehicleSimulation*>(VehicleSimulationPT.Get())->SetSimulationEnabled(bInEnable);
}

TUniquePtr<Chaos::FSimpleWheeledVehicle> UTrafficAIVehicleMovementComponent::CreatePhysicsVehicle()
{
	// Make the Vehicle Simulation class that will be updated from the physics thread async callback
	VehicleSimulationPT = MakeUnique<UTrafficAIVehicleSimulation>();

	return UChaosVehicleMovementComponent::CreatePhysicsVehicle();
}
