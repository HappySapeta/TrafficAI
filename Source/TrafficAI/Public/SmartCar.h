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

	virtual FVector GetVelocity() const override;

	virtual void Tick(float DeltaSeconds) override;

protected:

	virtual void BeginPlay() override;

private:
	
	void AlignWithVelocity();

private:

	UPROPERTY(EditAnywhere)
	TObjectPtr<UStaticMeshComponent> StaticMesh;

private:

	FVector Velocity;
	
	FVector PreviousLocation;
};
