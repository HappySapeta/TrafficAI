// Copyright Anupam Sahu. All Rights Reserved.

#include "Vehicles/TrafficAIVehicle.h"
#include "Components/SphereComponent.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"

// Sets default values
ATrafficAIVehicle::ATrafficAIVehicle()
{
	PrimaryActorTick.bCanEverTick = true;

	VehicleMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("VehicleMesh"));
	SetRootComponent(VehicleMesh);

	SetupWheel("WheelFL");
	SetupWheel("WheelFR");
	SetupWheel("WheelRL");
	SetupWheel("WheelRR");
}


void ATrafficAIVehicle::SetupWheel(const char* Suffix)
{
	USphereComponent* WheelCollider = CreateDefaultSubobject<USphereComponent>(*FString::Printf(TEXT("Collider_%hs"), Suffix));
	WheelCollider->SetupAttachment(VehicleMesh);
	WheelCollider->SetSimulatePhysics(true);
	WheelCollider->SetMassOverrideInKg(NAME_None, 25.0f);
	WheelCollider->SetEnableGravity(true);
	WheelCollider->SetCollisionObjectType(ECC_Vehicle);
	WheelCollider->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	WheelCollider->SetCollisionResponseToAllChannels(ECR_Block);
	WheelCollider->SetCollisionResponseToChannel(ECC_Vehicle, ECR_Ignore);

	const FString AxisConstraintName = FString::Printf(TEXT("AxisConstraint_%hs"), Suffix);
	UPhysicsConstraintComponent* AxisConstraint = CreateDefaultSubobject<UPhysicsConstraintComponent>(*AxisConstraintName);
	AxisConstraint->SetupAttachment(VehicleMesh);
	AxisConstraint->ComponentName1.ComponentName = *WheelCollider->GetName();
	AxisConstraint->ComponentName2.ComponentName = *VehicleMesh->GetName();
	AxisConstraint->SetLinearXLimit(LCM_Free, 0.0f);
	AxisConstraint->SetLinearYLimit(LCM_Free, 0.0f);
	AxisConstraint->SetLinearZLimit(LCM_Free, 0.0f);
	AxisConstraint->SetAngularSwing1Limit(ACM_Locked, 0.0);
	AxisConstraint->SetAngularSwing2Limit(ACM_Free, 0.0);
	AxisConstraint->SetAngularTwistLimit(ACM_Locked, 0.0);
	AxisConstraint->SetupAttachment(WheelCollider);

	const FString SuspensionConstraintName = FString::Printf(TEXT("SuspensionConstraint_%hs"), Suffix);
	UPhysicsConstraintComponent* SuspensionConstraint = CreateDefaultSubobject<UPhysicsConstraintComponent>(*SuspensionConstraintName);
	SuspensionConstraint->SetupAttachment(VehicleMesh);
	SuspensionConstraint->ComponentName1.ComponentName = *WheelCollider->GetName();
	SuspensionConstraint->ComponentName2.ComponentName = *VehicleMesh->GetName();
	SuspensionConstraint->SetLinearXLimit(LCM_Locked, 0.0f);
	SuspensionConstraint->SetLinearYLimit(LCM_Locked, 0.0f);
	SuspensionConstraint->SetLinearZLimit(LCM_Limited, 5.0f);
	SuspensionConstraint->SetAngularSwing1Limit(ACM_Free, 0.0);
	SuspensionConstraint->SetAngularSwing2Limit(ACM_Free, 0.0);
	SuspensionConstraint->SetAngularTwistLimit(ACM_Free, 0.0);
	SuspensionConstraint->SetupAttachment(WheelCollider);
	SuspensionConstraint->SetLinearDriveParams(2000.0f, 200.0f, 0.0f);

	WheelColliders.Add(WheelCollider);
	SuspensionConstraints.Add(SuspensionConstraint);
	AxisConstraints.Add(AxisConstraint);
}
