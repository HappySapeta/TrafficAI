#include "SmartCar.h"

#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"

// Sets default values
ASmartCar::ASmartCar()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	BoxComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxComponent"));
	SetRootComponent(BoxComponent);
	BoxComponent->SetCollisionProfileName(FName("BlockAllDynamic"));
	
	StaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMeshComponent"));
	StaticMesh->SetupAttachment(BoxComponent);
	StaticMesh->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
}

void ASmartCar::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateVelocity(DeltaSeconds);
	Move();
}

void ASmartCar::SetHeading(const FVector& NextWaypoint, const FVector& PreviousWaypoint)
{
	FVector FlatHeading = (NextWaypoint - GetActorLocation()).GetSafeNormal();
	FlatHeading.Z = 0;

	float Speed = Velocity.Size();
	SetActorRotation(UKismetMathLibrary::MakeRotFromX(FlatHeading), ETeleportType::ResetPhysics);
	SetActorLocation(FVector(PreviousWaypoint.X, PreviousWaypoint.Y, GetActorLocation().Z), false, NULL, ETeleportType::ResetPhysics);

	BoxComponent->SetPhysicsLinearVelocity(Speed * FlatHeading);
}

void ASmartCar::UpdateVelocity(float DeltaSeconds)
{
	const FVector& CurrentLocation = GetActorLocation();
	Velocity = (CurrentLocation - PreviousLocation) / DeltaSeconds;
	PreviousLocation = CurrentLocation;
}

void ASmartCar::Move()
{
	if(Velocity.GetSafeNormal().Dot(GetActorForwardVector()) < 0.0f && Acceleration < 0.0f)
	{
		return;
	}

	BoxComponent->AddForce(Acceleration * GetActorForwardVector(), NAME_None, true);
}