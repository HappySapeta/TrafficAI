#include "DrivingModel.h"
#include "SmartCar.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"

#define TRAFFIC_CHANNEL ECC_GameTraceChannel1

// Sets default values
ADrivingModel::ADrivingModel()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	SceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneComponent"));
	SetRootComponent(SceneComponent);
}

// Called when the game starts or when spawned

void ADrivingModel::BeginPlay()
{
	Super::BeginPlay();

	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ASmartCar::StaticClass(), SmartCars);
}

// Called every frame
void ADrivingModel::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	for(AActor* Element : SmartCars)
	{
		if(!IsValid(Element))
		{
			continue;
		}
		
		ASmartCar* SmartCar = static_cast<ASmartCar*>(Element);  
		const FVector& CurrentVelocity = SmartCar->GetVelocity();
		
		FHitResult HitResult;
		const FVector& RayStart = SmartCar->GetSensorLocation();
		const FVector& RayEnd = RayStart + SmartCar->GetActorForwardVector() * ModelData.MinimumGap;
		
		float CurrentGap = BIG_NUMBER;
		FVector RelativeVelocity = -CurrentVelocity;
		
		FCollisionQueryParams CollisionQueryParams;
		CollisionQueryParams.AddIgnoredActor(SmartCar);
		if(GetWorld()->LineTraceSingleByChannel(HitResult, RayStart, RayEnd, TRAFFIC_CHANNEL, CollisionQueryParams))
		{
			CurrentGap = FVector::Distance(HitResult.GetActor()->GetActorLocation(), SmartCar->GetActorLocation());
			RelativeVelocity += HitResult.GetActor()->GetVelocity();
			DrawDebugLine(GetWorld(), RayStart, RayEnd, FColor::Green);
		}
		else
		{
			DrawDebugLine(GetWorld(), RayStart, RayEnd, FColor::Red);
		}


		const float DesiredAcceleration = FMath::Clamp(IDM_Acceleration(CurrentVelocity.Size(), RelativeVelocity.Size(), CurrentGap), -ModelData.ComfortableBrakingDeceleration, ModelData.MaximumAcceleration);
		//const FVector& DesiredVelocity = FVector(CurrentVelocity.X, CurrentVelocity.Y, 0.0f) + SmartCar->GetActorForwardVector() * DesiredAcceleration * DeltaTime;
		//const FVector& DesiredLocation = SmartCar->GetActorLocation() + DesiredVelocity * DeltaTime * ModelData.SimulationSpeed;

		Cast<UBoxComponent>(SmartCar->GetComponentByClass(UBoxComponent::StaticClass()))->AddForce(DesiredAcceleration * SmartCar->GetActorForwardVector(), NAME_None, true);
		
		//UE_LOG(LogTemp, Warning, TEXT("Velocity = %s"), *CurrentVelocity.ToString());

		//SmartCar->SetActorLocation(DesiredLocation);
	}
}

float ADrivingModel::IDM_Acceleration(const float CurrentSpeed, const float RelativeSpeed, const float CurrentGap) const
{
	const float FreeRoadTerm = ModelData.MaximumAcceleration * (1 - FMath::Pow(CurrentSpeed / ModelData.DesiredSpeed, ModelData.AccelerationExponent));

	const float DecelerationTerm = (CurrentSpeed * RelativeSpeed) / (2 * FMath::Sqrt(ModelData.MaximumAcceleration * ModelData.ComfortableBrakingDeceleration));
	const float GapTerm = (ModelData.MinimumGap + ModelData.DesiredTimeHeadWay * CurrentSpeed + DecelerationTerm) / CurrentGap;
	
	const float InteractionTerm = -ModelData.MaximumAcceleration * FMath::Square(GapTerm);

	return FreeRoadTerm + InteractionTerm; 
}

