// Copyright Anupam Sahu. All Rights Reserved.


#include "Simulation/TrafficAISimulationSystem.h"

#include "Kismet/KismetSystemLibrary.h"
#include "Vehicles/TrafficAIVehicle.h"

void UTrafficAISimulationSystem::StartSimulation()
{
	RepSystem = GetWorld()->GetSubsystem<UTrafficAIRepresentationSystem>();
	Entities = RepSystem->GetEntities();

	FTimerDelegate SimTimerDelegate;
	SimTimerDelegate.BindUObject(this, &UTrafficAISimulationSystem::Simulate);
	GetWorld()->GetTimerManager().SetTimer(SimTimerHandle, SimTimerDelegate, TickRate, true);
}

void UTrafficAISimulationSystem::Simulate() const
{
	int Index = 0;
	TArray<float> RandomOffset {200, -100, 300};
	for (auto It = Entities.Pin()->begin(); It != Entities.Pin()->end(); ++It)
	{
		float DesiredAcceleration = BaseAcceleration + RandomOffset[Index++];

		AActor* EntityActor = (*It).Dummy;
		const float CurrentSpeed = EntityActor->GetVelocity().Length();

		const FVector& StartLocation = EntityActor->GetActorLocation() + EntityActor->GetActorUpVector() * 50.0f;
		const FVector& EndLocation = StartLocation + EntityActor->GetActorForwardVector() * LookAheadDistance;

		TArray ActorsToIgnore{EntityActor};

		FHitResult HitResult;
		if (UKismetSystemLibrary::LineTraceSingle(this, StartLocation, EndLocation, TraceTypeQuery3, false,
		                                          ActorsToIgnore, EDrawDebugTrace::ForDuration, HitResult, true,
		                                          FLinearColor::Red, FLinearColor::Green, 0.1f))
		{
			const float RelativeSpeed = (HitResult.GetActor()->GetVelocity() - EntityActor->GetVelocity()).Length();
			const float CurrentGap = HitResult.Distance;

			DesiredAcceleration = IDM_Acceleration(CurrentSpeed, RelativeSpeed, CurrentGap);
		}

		if (const ATrafficAIVehicle* Vehicle = Cast<ATrafficAIVehicle>(EntityActor))
		{
			Vehicle->GetRoot()->AddForce(DesiredAcceleration * EntityActor->GetActorForwardVector(), NAME_None, true);
		}
	}
}

float UTrafficAISimulationSystem::IDM_Acceleration(const float CurrentSpeed, const float RelativeSpeed, const float CurrentGap) const
{
	const float FreeRoadTerm = SimulationModel.MaximumAcceleration * (1 - FMath::Pow(CurrentSpeed / SimulationModel.DesiredSpeed, SimulationModel.AccelerationExponent));

	const float DecelerationTerm = (CurrentSpeed * RelativeSpeed) / (2 * FMath::Sqrt(SimulationModel.MaximumAcceleration * SimulationModel.ComfortableBrakingDeceleration));
	const float GapTerm = (SimulationModel.MinimumGap + SimulationModel.DesiredTimeHeadWay * CurrentSpeed + DecelerationTerm) / CurrentGap;

	const float InteractionTerm = -SimulationModel.MaximumAcceleration * FMath::Square(GapTerm);

	return FreeRoadTerm + InteractionTerm;
}
