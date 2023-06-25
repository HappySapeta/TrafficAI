#include "SmartCar.h"

#include "Kismet/KismetMathLibrary.h"

// Sets default values
ASmartCar::ASmartCar()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	StaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMeshComponent"));
	SetRootComponent(StaticMesh);
	StaticMesh->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
}

void ASmartCar::BeginPlay()
{
	Super::BeginPlay();
	PreviousLocation = GetActorLocation();
}

void ASmartCar::Tick(float DeltaSeconds)
 {
	Super::Tick(DeltaSeconds);
	
	const FVector& CurrentLocation = GetActorLocation();
	
	Velocity = (CurrentLocation - PreviousLocation) / DeltaSeconds;
	PreviousLocation = CurrentLocation;

	AlignWithVelocity();
 }

FVector ASmartCar::GetVelocity() const
{
	return Velocity;
}

void ASmartCar::AlignWithVelocity()
{
	const FRotator& Rotator = UKismetMathLibrary::MakeRotFromX(Velocity.GetSafeNormal());
	SetActorRotation(Rotator);
}
