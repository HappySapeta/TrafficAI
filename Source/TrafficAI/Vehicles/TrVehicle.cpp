// Copyright Anupam Sahu. All Rights Reserved.

#include "TrVehicle.h"
#include "TrafficAI/Simulation/TrSimulationSystem.h"

void ATrVehicle::BeginPlay()
{
	ThrottleController.Tune(ThrottleKp, ThrottleKi, ThrottleKd);
	SteeringController.Tune(SteeringKp, SteeringKi, SteeringKd);
	Super::BeginPlay();
}

void ATrVehicle::Tick(float DeltaSeconds)
{
	USkeletalMeshComponent* Root = GetMesh();
	
	const float PIDThrottle = ThrottleController.Evaluate(LocationError, DeltaSeconds);
	Root->AddForce(PIDThrottle * GetActorForwardVector(), NAME_None, true);

	Super::Tick(DeltaSeconds);
}

void ATrVehicle::SetTargetTransform(const FTransform& TargetTransform)
{
	LocationError = UTrSimulationSystem::ScalarProjection(TargetTransform.GetLocation() - GetActorLocation(), GetActorForwardVector());
	
	const FVector& CurrentHeading = GetActorForwardVector();
	const FVector& TargetHeading = TargetTransform.GetRotation().GetForwardVector();
	HeadingError = FMath::Atan2
	(
		CurrentHeading.X * TargetHeading.Y - CurrentHeading.Y * TargetHeading.X,
		CurrentHeading.X * TargetHeading.X + CurrentHeading.Y * TargetHeading.Y
	);
}
