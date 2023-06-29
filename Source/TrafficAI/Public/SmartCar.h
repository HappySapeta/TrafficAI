#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/BoxComponent.h"
#include "SmartCar.generated.h"

UCLASS()
class TRAFFICAI_API ASmartCar : public AActor
{
	GENERATED_BODY()

public:
	
	// Sets default values for this actor's properties
	ASmartCar();

	FVector GetSensorLocation() const { return BoxComponent->Bounds.Origin + GetActorForwardVector() * BoxComponent->Bounds.BoxExtent.X; }

	virtual FVector GetVelocity() const override { return Velocity; }

	void SetAcceleration(const float NewAcceleration) { Acceleration = NewAcceleration; }

	void SetHeading(const FVector& NextWaypoint, const FVector& PreviousWaypoint);

protected:
	
	virtual void Tick(float DeltaSeconds) override;

private:

	void Move();

	void UpdateVelocity(float DeltaSeconds);

private:

	UPROPERTY(EditAnywhere)
	TObjectPtr<UStaticMeshComponent> StaticMesh;

	UPROPERTY(EditAnywhere)
	TObjectPtr<class UBoxComponent> BoxComponent;

private:

	FVector Velocity;
	
	FVector PreviousLocation;

	FVector PreviousVelocity;

	float Acceleration;
};
