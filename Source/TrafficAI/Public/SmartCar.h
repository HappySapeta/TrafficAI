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
	
	void SetAcceleration(const float NewAcceleration) { Acceleration = NewAcceleration; }

	void SetHeading(const FVector& NewHeading) { Heading = NewHeading; }

protected:
	
	virtual void Tick(float DeltaSeconds) override;

private:

	void Move(const float DeltaTime);

	void Steer(const float DeltaTime);
	
	void ApplyTraction(float DeltaTime);

private:

	UPROPERTY(EditAnywhere)
	TObjectPtr<UStaticMeshComponent> StaticMesh;

	UPROPERTY(EditAnywhere)
	TObjectPtr<class UBoxComponent> BoxComponent;

	UPROPERTY(EditAnywhere, Category = "Movement Settings", meta = (UIMin = 0, ClampMin = 0))
	float SteeringProportionalCoefficient = 1.0f;
	
	UPROPERTY(EditAnywhere, Category = "Movement Settings", meta = (UIMin = 0, ClampMin = 0))
	float SteeringDerivativeCoefficient = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Movement Settings", meta = (UIMin = 0, ClampMin = 0))
	float Traction = 1.0f;

#if WITH_EDITORONLY_DATA
	UPROPERTY(EditAnywhere, Category = "Debug Settings")
	bool bDebugSpeed = false;
#endif
	
private:

	FVector Heading;

	float Acceleration;
	float PreviousTheta;
};
