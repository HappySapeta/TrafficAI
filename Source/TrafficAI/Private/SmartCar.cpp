#include "SmartCar.h"

// Sets default values
ASmartCar::ASmartCar()
	:PreviousTheta(0)
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

	Move(DeltaSeconds);
	Steer(DeltaSeconds);
	ApplyTraction(DeltaSeconds);

#if WITH_EDITORONLY_DATA
	if(bDebugSpeed)
	{
		GEngine->AddOnScreenDebugMessage(-1, DeltaSeconds, FColor::Emerald, FString::Printf(TEXT("Speed = %f km/h"), GetVelocity().Size() * 0.01f));
	}
#endif
}

void ASmartCar::Move(const float DeltaTime)
{
	if(GetVelocity().GetSafeNormal().Dot(GetActorForwardVector()) < 0.0f && Acceleration < 0.0f)
	{
		return;
	}

	BoxComponent->AddForce(Acceleration * GetActorForwardVector(), NAME_None, true);
}

void ASmartCar::Steer(const float DeltaTime)
{
	FVector HeadingDirection = (Heading - GetActorLocation()).GetSafeNormal();
	HeadingDirection.Z = 0.0f;
	
	FVector Forward = GetActorForwardVector();
	Forward.Z = 0.0f;
	
	float Theta = FMath::Atan2(Forward.X * HeadingDirection.Y - Forward.Y * HeadingDirection.X, Forward.X * HeadingDirection.X + Forward.Y * HeadingDirection.Y);
	
	float P = SteeringProportionalCoefficient * Theta;
	float D = SteeringDerivativeCoefficient * (Theta - PreviousTheta) / DeltaTime;
	float Torque = P + D;
	PreviousTheta = Theta;

	BoxComponent->AddTorqueInRadians(Torque * GetActorUpVector(), NAME_None, true);
}

void ASmartCar::ApplyTraction(const float DeltaTime)
{
	const FVector& LocalVelocity = GetActorTransform().InverseTransformVector(GetVelocity());
	BoxComponent->AddForce( -1 * GetActorRightVector() * LocalVelocity.Y * Traction, NAME_None, true);
}