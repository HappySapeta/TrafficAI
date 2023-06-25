#include "DrivingModel.h"

#include "SmartCar.h"


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
}

float ADrivingModel::CalculateAcceleration(const float CurrentSpeed, const float RelativeSpeed, const float CurrentGap) const
{
	const float FreeRoadTerm = FMath::Pow(CurrentSpeed / ModelData.DesiredSpeed, ModelData.AccelerationExponent);

	const float T1 = ModelData.MinimumGap;
	const float T2 = CurrentSpeed * ModelData.DesiredTimeHeadWay;
	const float T3 = (CurrentSpeed * RelativeSpeed) / (2 * FMath::Sqrt(ModelData.MaximumAcceleration * ModelData.ComfortableBrakingDeceleration));
	
	const float DesiredGap = T1 + T2 + T3;
	const float InteractionTerm = FMath::Square(DesiredGap / CurrentGap);

	const float Acceleration = ModelData.MaximumAcceleration * (1 - FreeRoadTerm - InteractionTerm);

	return Acceleration;
}

// Called every frame
void ADrivingModel::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	for(int Current = 0; Current < SmartCars.Num(); ++Current)
	{
		const FVector& CurrentVelocity = SmartCars[Current]->GetVelocity();
		//UE_LOG(LogTemp, Warning, TEXT("%d Speed : %f km/h"), Current, CurrentVelocity.Size() * 0.036f);

		FVector RelativeVelocity = -CurrentVelocity;
		float CurrentGap = BIG_NUMBER;
		if(Current != SmartCars.Num() - 1)
		{
			RelativeVelocity += SmartCars[Current + 1]->GetVelocity();
			CurrentGap = (SmartCars[Current + 1]->GetActorLocation() - SmartCars[Current]->GetActorLocation()).Size() - ModelData.CarLength;
		}
		
		const float DesiredAcceleration = CalculateAcceleration(CurrentVelocity.Size(), RelativeVelocity.Size(), CurrentGap);
		const FVector& DesiredVelocity = (CurrentVelocity.Size() + DesiredAcceleration * DeltaTime) * SmartCars[Current]->GetActorForwardVector();
		const FVector& DesiredLocation = SmartCars[Current]->GetActorLocation() + DesiredVelocity * DeltaTime * 1.0f;
		
		SmartCars[Current]->SetActorLocation(DesiredLocation);
	}
}

