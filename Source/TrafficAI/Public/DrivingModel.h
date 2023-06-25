﻿#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DrivingModel.generated.h"

USTRUCT(BlueprintType)
struct FModelData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	float DesiredSpeed = 3000;

	UPROPERTY(EditAnywhere)
	float MinimumGap = 200;

	UPROPERTY(EditAnywhere)
	float DesiredTimeHeadWay = 1.5f;

	UPROPERTY(EditAnywhere)
	float MaximumAcceleration = 73.0f;

	UPROPERTY(EditAnywhere)
	float ComfortableBrakingDeceleration = 167.0f;

	UPROPERTY(EditAnywhere)
	float AccelerationExponent = 4.0f;

	UPROPERTY(EditAnywhere)
	float CarLength = 500;
	
	UPROPERTY(EditAnywhere)
	float LeaderSpeed = 3000.0f;
};


UCLASS()
class TRAFFICAI_API ADrivingModel : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ADrivingModel();

	// Called every frame
	virtual void Tick(float DeltaTime) override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

private:
	float CalculateAcceleration(
		const float CurrentSpeed, const float RelativeSpeed, const float CurrentGap) const;
	
private:

	UPROPERTY(EditAnywhere, Category = "Simulation Setup")
	FModelData ModelData;

	UPROPERTY(EditAnywhere, Category = "Simulation Setup")
	TArray<class ASmartCar*> SmartCars;

	UPROPERTY(EditAnywhere)
	float Radius = 2000.0f;

	UPROPERTY()
	TObjectPtr<USceneComponent> SceneComponent;
	
};