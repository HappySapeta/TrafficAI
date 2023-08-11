// Copyright Anupam Sahu. All Rights Reserved.

#include "TrafficRepresentationSystem/TrafficAIRepresentationSystem.h"
#include "TrafficAICommonTypes.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Kismet/KismetMathLibrary.h"

ATrafficAIRepresentationSystem::ATrafficAIRepresentationSystem()
{
	PrimaryActorTick.bCanEverTick = true;
	ISMC = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("InstancedStaticMeshComponent"));
	SetRootComponent(ISMC);
}

void ATrafficAIRepresentationSystem::DebugGenerateRandomInstances()
{
	constexpr float Range = 10000.0f;
	
	TArray<FTransform> Transforms;
	for(int Index = 0; Index < NumberOfInstances; ++Index)
	{
		const FVector& RandomLocation = UKismetMathLibrary::RandomUnitVector() * FMath::RandRange(0.0f, Range);
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
	InstanceTransforms = Transforms;
	ISMC->AddInstances(InstanceTransforms, false, true);
	ISMC->SetMaterial(0, ISMC_Material);

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
			SET_ACTOR_ENABLED(NewActor, false)
			ActorInstances.Add(NewActor);
		}
		
		++InstanceIndex;
	}
}

void ATrafficAIRepresentationSystem::BeginPlay()
{
	Super::BeginPlay();
	LocalPlayer = GetWorld()->GetFirstPlayerController()->GetPawn();
}

void ATrafficAIRepresentationSystem::Tick(float DeltaSeconds)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(ATrafficAIRepresentationSystem::Tick)

	constexpr float ROTATION_SPEED = 180.0f;

	if(InstanceTransforms.Num() == 0)
	{
		return;
	}

	for(int Index = 0; Index < InstanceTransforms.Num(); ++Index)
	{
		const float Distance = FVector::Distance(InstanceTransforms[Index].GetLocation(), LocalPlayer->GetActorLocation());
		
		const bool bIsLOVisible = L0_Range.Contains(Distance);
		const bool bIsL1Visible = L1_Range.Contains(Distance); 

		const FVector NewScale = bIsLOVisible ? FVector::One() : FVector::ZeroVector;
		InstanceTransforms[Index].SetScale3D(NewScale);

		const FRotator& NewRotator = InstanceTransforms[Index].Rotator() + FRotator(0.0f, ROTATION_SPEED * DeltaSeconds, 0.0f);
		InstanceTransforms[Index].SetRotation(NewRotator.Quaternion());

		if(bIsL1Visible)
		{
			ActorInstances[Index]->SetActorRotation(NewRotator);
		}
		SET_ACTOR_ENABLED(ActorInstances[Index], bIsL1Visible)
	}

	ISMC->BatchUpdateInstancesTransforms(0, InstanceTransforms, true, false);
	ISMC->BatchUpdateInstancesTransform(InstanceTransforms.Num() - 1, 1, InstanceTransforms.Last(), true, true);
	
	Super::Tick(DeltaSeconds);
}
