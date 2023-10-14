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
	USphereComponent* NewSphereCollider = CreateDefaultSubobject<USphereComponent>(*FString::Printf(TEXT("Collider_%hs"), Suffix));
	NewSphereCollider->SetupAttachment(VehicleMesh);
		
	const FString AxisConstraintName = FString::Printf(TEXT("AxisConstraint_%hs"), Suffix);
	UPhysicsConstraintComponent* NewAxisConstraint = CreateDefaultSubobject<UPhysicsConstraintComponent>(*AxisConstraintName);
	NewAxisConstraint->SetupAttachment(VehicleMesh);
	NewAxisConstraint->ComponentName1.ComponentName = *NewSphereCollider->GetName();
	NewAxisConstraint->ComponentName2.ComponentName = *VehicleMesh->GetName();
	NewAxisConstraint->SetLinearXLimit(LCM_Free, 0.0f);
	NewAxisConstraint->SetLinearYLimit(LCM_Free, 0.0f);
	NewAxisConstraint->SetLinearZLimit(LCM_Free, 0.0f);
	NewAxisConstraint->SetAngularSwing1Limit(ACM_Locked, 0.0);
	NewAxisConstraint->SetAngularSwing2Limit(ACM_Free, 0.0);
	NewAxisConstraint->SetAngularTwistLimit(ACM_Locked, 0.0);
	NewAxisConstraint->SetupAttachment(NewSphereCollider);
	
	const FString SuspensionConstraintName = FString::Printf(TEXT("SuspensionConstraint_%hs"), Suffix);
	UPhysicsConstraintComponent* NewSuspensionConstraint = CreateDefaultSubobject<UPhysicsConstraintComponent>(*SuspensionConstraintName);
	NewSuspensionConstraint->SetupAttachment(VehicleMesh);
	NewSuspensionConstraint->ComponentName1.ComponentName = *NewSphereCollider->GetName();
	NewSuspensionConstraint->ComponentName2.ComponentName = *VehicleMesh->GetName();
	NewSuspensionConstraint->SetLinearXLimit(LCM_Locked, 0.0f);
	NewSuspensionConstraint->SetLinearYLimit(LCM_Locked, 0.0f);
	NewSuspensionConstraint->SetLinearZLimit(LCM_Limited, 5.0f);
	NewSuspensionConstraint->SetAngularSwing1Limit(ACM_Free, 0.0);
	NewSuspensionConstraint->SetAngularSwing2Limit(ACM_Free, 0.0);
	NewSuspensionConstraint->SetAngularTwistLimit(ACM_Free, 0.0);
	NewSuspensionConstraint->SetupAttachment(NewSphereCollider);

	SphereColliders.Add(NewSphereCollider);
	SuspensionConstraints.Add(NewSuspensionConstraint);
	AxisConstraints.Add(NewAxisConstraint);
}
