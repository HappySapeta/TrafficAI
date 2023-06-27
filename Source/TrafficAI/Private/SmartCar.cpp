#include "SmartCar.h"

#include "Components/BoxComponent.h"

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

void ASmartCar::BeginPlay()
{
	Super::BeginPlay();
}

void ASmartCar::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const FVector& CurrentLocation = GetActorLocation();
	Velocity = (CurrentLocation - PreviousLocation) / DeltaSeconds;
	PreviousLocation = CurrentLocation;
}

FVector ASmartCar::GetSensorLocation() const
{
	return BoxComponent->Bounds.Origin + GetActorForwardVector() * BoxComponent->Bounds.BoxExtent.X;
}

FVector ASmartCar::GetVelocity() const
{
	return Velocity;
}

void ASmartCar::AddForce(const FVector& Force) const
{
	BoxComponent->AddForce(Force, NAME_None, true);
}
