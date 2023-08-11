// Copyright Anupam Sahu. All Rights Reserved.

#include "TrafficRepresentationSystem/TrafficAIRepresentationSystem.h"
#include "TrafficAICommonTypes.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Kismet/KismetMathLibrary.h"

ATrafficAIRepresentationSystem::ATrafficAIRepresentationSystem()
{
	ISMC = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("InstancedStaticMeshComponent"));
	SetRootComponent(ISMC);
}

void ATrafficAIRepresentationSystem::DebugGenerateRandomInstances()
{
	constexpr int NumberOfInstances = 100;
	constexpr float Range = 10000.0f;
	
	TArray<FTransform> Transforms;
	for(int Index = 0; Index < NumberOfInstances; ++Index)
	{
		const FVector& RandomLocation = UKismetMathLibrary::RandomUnitVector() * Range;
		const FRotator& RandomRotator = UKismetMathLibrary::RandomRotator();
		Transforms.Add(FTransform(RandomRotator, RandomLocation, FVector::One()));
	}

	GenerateInstances(SampleDistribution, Transforms, NumberOfInstances);
}

void ATrafficAIRepresentationSystem::DebugClearInstances()
{
	for(AActor* Instance : ActorInstances)
	{
		Instance->Destroy();
	}

	ISMC->ClearInstances();

	ActorInstances.Reset();
}

void ATrafficAIRepresentationSystem::GenerateInstances(FTrafficRepresentation Distribution, const TArray<FTransform>& Transforms, int MaxCount)
{
	ISMC->SetStaticMesh(Distribution.StaticMesh);
	ISMC->AddInstances(Transforms, false, true);

	UWorld* World = GetWorld();
	
	int InstanceIndex = 0;
	while(InstanceIndex < Transforms.Num() && InstanceIndex < MaxCount)
	{
		const FTransform& NewTransform = Transforms[InstanceIndex];

		FActorSpawnParameters SpawnParameters;
#if WITH_EDITOR
		SpawnParameters.bHideFromSceneOutliner = true;
#endif
		
		AActor* NewActor = World->SpawnActor(Distribution.Dummy, &NewTransform, SpawnParameters);
		if(IsValid(NewActor))
		{
			DISABLE_ACTOR(NewActor)
			ActorInstances.Add(NewActor);
		}
		
		++InstanceIndex;
	}
}
