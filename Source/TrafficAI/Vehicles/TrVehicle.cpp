// Copyright Anupam Sahu. All Rights Reserved.

#include "TrVehicle.h"

#include "TrafficAI/Simulation/TrSimulationSystem.h"

ATrVehicle::ATrVehicle()
{
	PrimaryActorTick.bCanEverTick = true;
	AccelerationController = FRpPIDController<float>(&Acceleration, 0.5f, 0.0f, 0.1f);
}

void ATrVehicle::Tick(float DeltaSeconds)
{
	const FVector& DesiredHeading = GetActorForwardVector();
	
	const float PIDAcceleration = AccelerationController.Evaluate(DesiredAcceleration, DeltaSeconds);
	GetMesh()->AddForce(PIDAcceleration * DesiredHeading, NAME_None, true);

	const FVector& CurrentVelocity = GetMesh()->GetPhysicsLinearVelocity();
	const FVector& VelocityDifference = CurrentVelocity - PreviousVelocity;
	Acceleration = UTrSimulationSystem::ScalarProjection(VelocityDifference, DesiredHeading);

	PreviousVelocity = CurrentVelocity;

	GEngine->AddOnScreenDebugMessage
	(
		-1, DeltaSeconds, FColor::Red,
		FString::Printf(TEXT("Current = %.1f, Desired = %.1f"), Acceleration, DesiredAcceleration),
		true, FVector2D::One() * 3.0f
	);
	
	Super::Tick(DeltaSeconds);
}

void ATrVehicle::SetAcceleration(const float DesiredValue)
{
	DesiredAcceleration = DesiredValue;
}

void ATrVehicle::SetHeading(const FVector& DesiredHeading)
{
}
