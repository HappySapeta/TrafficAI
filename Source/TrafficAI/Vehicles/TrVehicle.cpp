// Copyright Anupam Sahu. All Rights Reserved.

#include "TrVehicle.h"

void ATrVehicle::BeginPlay()
{
	ThrottleController.Tune(ThrottleKp, ThrottleKi, ThrottleKd);
	SteeringController.Tune(SteeringKp, SteeringKi, SteeringKd);
	Super::BeginPlay();
}

void ATrVehicle::Tick(float DeltaSeconds)
{
	USkeletalMeshComponent* Root = GetMesh();
	const FVector VecToTarget = DesiredTransform.GetLocation() - Root->GetComponentLocation();
	const float LocationError = VecToTarget.Length();

	const float PIDThrust = ThrottleController.Evaluate(LocationError, DeltaSeconds);
	Root->AddForce(PIDThrust * VecToTarget.GetSafeNormal(), NAME_None, true);
	
	Super::Tick(DeltaSeconds);
}

void ATrVehicle::SetDesiredTransform(const FTransform& Transform)
{
	DesiredTransform = Transform;
}
