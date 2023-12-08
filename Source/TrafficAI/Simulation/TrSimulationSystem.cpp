// Copyright Anupam Sahu. All Rights Reserved.


#include "TrSimulationSystem.h"

#include "Async/ParallelFor.h"

void UTrSimulationSystem::RegisterEntites(TWeakPtr<TArray<FTrEntity>> NewEntities)
{
	Entities = NewEntities;
	RegisterEntities_Internal();
}

void UTrSimulationSystem::RegisterEntities_Internal()
{
	if(!Entities.IsValid())
	{
		return;
	}

	const uint32 NumEntities = Entities.Pin()->Num();
	ParallelFor(NumEntities, [this](const uint32 Index)
	{
		const AActor* Dummy = Entities.Pin()->operator[](Index).Dummy;
		FVector NewPosition = Dummy->GetActorLocation();
		Positions.Add(
			{
				static_cast<float>(NewPosition.X),
				static_cast<float>(NewPosition.Y)
			});

		FVector NewHeading = Dummy->GetActorForwardVector();
		Headings.Add(
			{
				static_cast<float>(NewHeading.X),
				static_cast<float>(NewHeading.Y)
			});

		float NewSpeed = Dummy->GetVelocity().Length();
		Speeds.Add(NewSpeed);
		
	});

	for(uint32 Index = 0; Index < NumEntities; ++Index)
	{
		DrawDebugPoint(GetWorld(), FVector(Positions[Index].X, Positions[Index].Y, 500.0f), 10.0f, FColor::Blue, true);
	}
}
