// Copyright Anupam Sahu. All Rights Reserved.

#include "TrTrafficSimulator.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "TrafficAI/Representation/TrRepresentationSystem.h"
#include "TrafficAI/Representation/TrISMCManager.h"
#include "TrafficAI/Vehicles/TrVehicle.h"

void UTrTrafficSimulator::StartSimulation(const float TickRate, const float Acceleration, const float MaxSpeed, const FVector Location, const float MaxLoopDistance)
{
	FTimerDelegate SimTimerDelegate;
	SimTimerDelegate.BindUObject(this, &UTrTrafficSimulator::DoSimulation);

	const UTrRepresentationSystem* RepresentationSystem = GetWorld()->GetSubsystem<UTrRepresentationSystem>();
	Entities = RepresentationSystem->GetEntities();
	Visualizer = RepresentationSystem->GetTrafficVisualizer();

	DesiredAcceleration = Acceleration;
	DesiredMaxSpeed = MaxSpeed;
	DeltaTime = TickRate;
	LoopingDistance = MaxLoopDistance;
	StartingLocation = Location;
	
	GetWorld()->GetTimerManager().SetTimer(SimTimerHandle, SimTimerDelegate, TickRate, true);
}

void UTrTrafficSimulator::DoSimulation()
{
	FTrEntity& Entity = *Entities.Pin()->begin();

	switch(Entity.LODLevel)
	{
	case ELODLevel::LOD0:
		{
			ATrVehicle* Vehicle = Cast<ATrVehicle>(Entity.Dummy);
			UPrimitiveComponent* VehicleRoot = Vehicle->GetRoot();
			
			Vehicle->SetComplexSimulationEnabled(false);

			if(Entity.PreviousLODLevel == ELODLevel::LOD1)
			{
				FTransform VehicleTransform;
				Visualizer->GetISMC(Entity.Mesh)->GetInstanceTransform(Entity.InstanceIndex, VehicleTransform, true);
				
				VehicleRoot->SetWorldLocation(VehicleTransform.GetLocation(), false, nullptr, ETeleportType::ResetPhysics);
				Entity.PreviousLODLevel = ELODLevel::LOD0;
			}
			
			VehicleRoot->AddForce(VehicleRoot->GetForwardVector() * DesiredAcceleration, NAME_None, true);
				
			const float ForwardSpeed = FVector::DotProduct(VehicleRoot->GetComponentVelocity(), VehicleRoot->GetForwardVector()); 
			if(ForwardSpeed >= DesiredMaxSpeed)
			{
				const FVector NewVelocity = VehicleRoot->GetForwardVector() * DesiredMaxSpeed;
				VehicleRoot->SetPhysicsLinearVelocity(NewVelocity);
			}

			if(FVector::Dist2D(Vehicle->GetActorLocation(), StartingLocation) >= LoopingDistance)
			{
				Vehicle->SetActorLocation(StartingLocation, false, nullptr, ETeleportType::TeleportPhysics);
			}
			
			break;
		}

	case ELODLevel::LOD1:
		{
			if(Entity.PreviousLODLevel == ELODLevel::LOD0)
			{
				FTransform VehicleTransform;
				Visualizer->GetISMC(Entity.Mesh)->GetInstanceTransform(Entity.InstanceIndex, VehicleTransform, true);

				VehicleTransform.SetLocation(Entity.Dummy->GetActorLocation());
				
				Visualizer->GetISMC(Entity.Mesh)->UpdateInstanceTransform(Entity.InstanceIndex, VehicleTransform, true, true);
				Entity.PreviousLODLevel = ELODLevel::LOD1;
			}
		
			FTransform VehicleTransform;
			Visualizer->GetISMC(Entity.Mesh)->GetInstanceTransform(Entity.InstanceIndex, VehicleTransform, true);

			FVector NewVelocity = PreviousVelocity + DesiredAcceleration * PreviousVelocity.GetSafeNormal() * DeltaTime;
			if(NewVelocity.Dot(PreviousVelocity.GetSafeNormal()) >= DesiredMaxSpeed)
			{
				NewVelocity = PreviousVelocity.GetSafeNormal() * DesiredMaxSpeed;	
			}
			
			VehicleTransform.SetLocation(VehicleTransform.GetLocation() + NewVelocity * DeltaTime);

			PreviousVelocity = NewVelocity;


			if(FVector::Dist2D(VehicleTransform.GetLocation(), StartingLocation) >= LoopingDistance)
			{
				VehicleTransform.SetLocation(StartingLocation);
			}

			Visualizer->GetISMC(Entity.Mesh)->UpdateInstanceTransform(Entity.InstanceIndex, VehicleTransform, true, true);
			
			break;
		}

		default:
		{
			return;
		}
	}
}
