#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SmartCar.generated.h"

UCLASS()
class TRAFFICAI_API ASmartCar : public AActor
{
	GENERATED_BODY()

public:
	
	// Sets default values for this actor's properties
	ASmartCar();

	FVector GetSensorLocation() const;

	virtual FVector GetVelocity() const override;

protected:

	virtual void BeginPlay() override;
	
	virtual void Tick(float DeltaSeconds) override;

private:

	UPROPERTY(EditAnywhere)
	TObjectPtr<UStaticMeshComponent> StaticMesh;

	UPROPERTY(EditAnywhere)
	TObjectPtr<class UBoxComponent> BoxComponent;

private:

	FVector Velocity;
	
	FVector PreviousLocation;

	FVector PreviousVelocity;

	FVector Acceleration;
};
