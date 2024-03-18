// Copyright Anupam Sahu. All Rights Reserved.

#include "TrVehicle.h"

#include "ChaosVehicleMovementComponent.h"

void ATrVehicle::BeginPlay()
{
	ThrottleController.Tune(ThrottleKp, ThrottleKi, ThrottleKd);
	SteeringController.Tune(SteeringKp, SteeringKi, SteeringKd);
	Super::BeginPlay();
}

void ATrVehicle::Tick(float DeltaSeconds)
{
	USkeletalMeshComponent* Root = GetMesh();
	UChaosVehicleMovementComponent* VehicleMovement = GetVehicleMovementComponent();

	const FVector VecToTarget = DesiredTransform.GetLocation() - Root->GetComponentLocation();
	const float LocationError = VecToTarget.Length();

	const float PIDThrust = ThrottleController.Evaluate(LocationError, DeltaSeconds);
	Root->AddForce(PIDThrust * VecToTarget.GetSafeNormal(), NAME_None, true);

	const FVector& DesiredHeading = DesiredTransform.GetRotation().GetForwardVector();
	const FVector& CurrentHeading = GetActorForwardVector();

	const FVector& TurningAxis = FVector::UpVector;
	const float HeadingError = FMath::Acos(DesiredHeading.Dot(CurrentHeading)) * -1 * FMath::Sign((DesiredHeading.Cross(CurrentHeading).Dot(TurningAxis)));
	const float PIDSteering = SteeringController.Evaluate(HeadingError, DeltaSeconds);
	VehicleMovement->SetSteeringInput(PIDSteering);
	
	Super::Tick(DeltaSeconds);
}

void ATrVehicle::OnActivated(const FTransform& Transform, const FVector& Velocity)
{
	SetActorTransform(Transform, false, nullptr, ETeleportType::TeleportPhysics);
	GetMesh()->SetPhysicsLinearVelocity(Velocity);
	DesiredTransform = Transform;
}

void ATrVehicle::SetDesiredTransform(const FTransform& Transform)
{
	DesiredTransform = Transform;
}

void ATrVehicle::PossessedBy(AController* NewController)
{
	OnPossessed.Broadcast();
	Super::PossessedBy(NewController);
}
