// Copyright Anupam Sahu. All Rights Reserved.

#include "TrVehicleMovementComponent.h"

void UTrVehicleSimulation::SetSimulationEnabled(const bool bInEnable)
{
	bIsSimEnabled = bInEnable;
}

void UTrVehicleSimulation::ApplyInput(const FControlInputs& ControlInputs, float DeltaTime)
{
	if(!bIsSimEnabled)
	{
		return;
	}
	UChaosWheeledVehicleSimulation::ApplyInput(ControlInputs, DeltaTime);
}

void UTrVehicleSimulation::ProcessMechanicalSimulation(float DeltaTime)
{
	if(!bIsSimEnabled)
	{
		return;
	}
	UChaosWheeledVehicleSimulation::ProcessMechanicalSimulation(DeltaTime);
}

void UTrVehicleSimulation::ProcessSteering(const FControlInputs& ControlInputs)
{
	if(!bIsSimEnabled)
	{
		return;
	}
	UChaosWheeledVehicleSimulation::ProcessSteering(ControlInputs);
}

void UTrVehicleMovementComponent::SetSimulationEnabled(const bool bInEnable)
{
	static_cast<UTrVehicleSimulation*>(VehicleSimulationPT.Get())->SetSimulationEnabled(bInEnable);
}

TUniquePtr<Chaos::FSimpleWheeledVehicle> UTrVehicleMovementComponent::CreatePhysicsVehicle()
{
	// Make the Vehicle Simulation class that will be updated from the physics thread async callback
	VehicleSimulationPT = MakeUnique<UTrVehicleSimulation>();

	return UChaosVehicleMovementComponent::CreatePhysicsVehicle();
}
