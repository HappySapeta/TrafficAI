// Copyright Anupam Sahu. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FTrIntersectionManager.h"
#include "TrSimulationData.h"
#include "TrTypes.h"
#include "Ripple/Public/RpSpatialGraphComponent.h"
#include "SpatialAcceleration/RpImplicitGrid.h"
#include "TrafficAI/Utility/TrSpatialGraphComponent.h"
#include "TrSimulationSystem.generated.h"

class UTrSimulationConfiguration;

/**
 * @class UTrSimulationSystem
 *
 * This class represents the simulation system in the traffic AI module.
 *
 * It is responsible for managing and updating the simulation state of all vehicles.
 * It uses graph component and a collection of traffic entities to simulate
 * the movement of vehicles on a spatial graph.
 * It tracks the positions, velocities, headings, goals, and the current paths of the vehicles.
 *
 * The simulation system uses the Intelligent Driver Model to drive the vehicles,
 * and the Kinematic Bicycle Model to steer the vehicles.
 * It also uses Craig Reynold's path following algorithm to keep the vehicles on track.
 * 
 * A Spatial Acceleration Structure called an Implicit Grid,
 * allows it to query obstacles near a vehicle, at tremendous speeds.
 */
UCLASS()
class TRAFFICAI_API UTrSimulationSystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/**
	 * @brief Initialize the simulation system.
	 *
	 * This method initializes the simulation system with the provided configuration and data.
	 *
	 * @param SimData Pointer to the simulation configuration data.
	 * @param GraphComponent Pointer to the spatial graph component used for simulation.
	 * @param TrafficEntities Array of vehicle representations used for simulation.
	 * @param TrafficVehicleStarts Array of vehicle path transforms for initial vehicle positions.
	 *
	 * @details This method initializes the simulation system with the provided simulation configuration
	 * and data. It sets various parameters, initializes the spatial acceleration structure, and
	 * sets up timers for traffic signal switching.
	 * 
	 * @note This method assumes that the SimData and GraphComponent
	 * parameters are not null and have valid values.
	 */
	void Initialize
	(
		const UTrSimulationConfiguration* SimData,
		const UTrSpatialGraphComponent* GraphComponent,
		const TArray<FTrVehicleRepresentation>& TrafficEntities,
		const TArray<FTrVehiclePathTransform>& TrafficVehicleStarts
	);

	// No implementation required here.
	void Initialize(FSubsystemCollectionBase& Collection) override {};

	/**
	 * @brief Update the simulation state of the vehicles.
	 *
	 * This method updates the simulation state of all vehicles in the simulation system.
	 * It works by calling functions that perform dedicated tasks for the simulation.
	 */
	void TickSimulation(const float DeltaSeconds);

	/**
	 * @brief Retrieve the transforms of the vehicles.
	 *
	 * This method retrieves the transforms of the vehicles in the simulation system.
	 * It fills the provided array with the current positions and orientations of the vehicles.
	 * The positions are relative to the provided position offset.
	 */
	void GetVehicleTransforms(TArray<FTransform>& OutTransforms, const FVector& PositionOffset);

	/**
	 * @brief Begin the destruction sequence for the simulation system.
	 *
	 * This method is called when the simulation system is being destroyed
	 * and is responsible for clearing any timers set in the simulation system.
	 */
	virtual void BeginDestroy() override;

protected:

#pragma region Utility
	
	/**
	 * @brief Projects a point onto a given path and returns the clamped projection point.
	 *
	 * This method takes a point and a path, and projects the point onto the path.
	 * The resulting projection point is clamped to within the path's start and end points.
	 *
	 * @return The clamped projection point on the path.
	 */
	FVector ProjectPointOnPathClamped(const FVector& Point, const FTrPath& Path) const;

	/**
	 * @brief Find the nearest path to a given entity in the simulation system.
	 *
	 * This method finds the nearest path to the entity with the specified index.
	 * It calculates the distance between the entity's current position and its projection on each path in the simulation system.
	 * The index of the path closest to the entity is returned as the nearest path.
	 */
	int FindNearestPath(int EntityIndex, FVector& NearestProjection) const;

	/**
	 * @brief Calculates the scalar projection of one vector onto another.
	 *
	 * This method takes two vectors, `V1` and `V2`, and calculates the scalar projection of `V1` onto `V2`.
	 * The scalar projection is defined as the length of the projection of `V1` onto `V2` when `V2` is used as the reference vector.
	 */
	float ScalarProjection(const FVector& V1, const FVector& V2) const { return V1.Dot(V2) / V2.Length(); }
	
#pragma endregion

#pragma region Debug
#if !UE_BUILD_SHIPPING
	void DrawDebug();
	void DrawGraph(const UWorld* World);
#endif
#pragma endregion

private:
	/**
	 * @brief Sets the goals for each entity in the simulation system.
	 *
	 * This method calculates and sets the goals for each entity in the simulation system.
	 * The goals are used to determine the desired path or destination for each entity.
	 *
	 * This method uses a look-ahead distance to predict the future position of each entity.
	 * It then calculates the future position on the path for each entity, based on the current path and offset.
	 * If the distance between the current position and the position on the path is less than a threshold,
	 * the goal is set to the end point of the path and the path following state is set to true.
	 * Otherwise, the goal is set to the future position on the path and the path following state is set to false.
	 */
	void SetGoals();

	/**
	 * @brief Handles updating the goals of the vehicles in the simulation system.
	 *
	 * This method is responsible for updating the goals of the vehicles in the simulation system.
	 * It checks the distance between the current position and the assigned goal for each vehicle.
	 * If the distance is less than or equal to the specified goal update distance,
	 * and the vehicle is in a path following state, it calls the UpdatePath method for that vehicle.
	 */
	void HandleGoals();

	/**
	 * @brief Update the kinematics of all vehicles in the simulation system.
	 *
	 * This method is responsible for updating the positions and velocities of all vehicles
	 * based on their current positions, velocities, and accelerations.
	 *
	 * It uses IDM to determine the target acceleration of each vehicle.
	 * IDM requires a target to calculate the acceleration.
	 * The target is either the goal or the leading vehicle based on whichever is closer.
	 */
	void UpdateKinematics();

	/**
	 * @brief Updates the orientations of all entities in the simulation.
	 *
	 * This method is responsible for updating the orientations of all entities in the simulation.
	 * It calculates the new heading, position, and velocity for each entity based on its goal and current parameters.
	 * The entities' positions are updated using the Kinematic Bicycle Model, which takes into account the current heading, velocity, and steering angle.
	 * The steering angle is calculated based on the desired direction of travel and the current heading.
	 */
	void UpdateOrientations();

	/**
	 * @brief Updates the collision data for all vehicles in the simulation system.
	 *
	 * This method is called to update the collision data for all vehicles in the simulation system.
	 * It uses the ImplicitGrid spatial acceleration structure to query for obstacles near each vehicle,
	 * and updates the LeadingVehicleIndices array based on the closest vehicle ahead.
	 */
	void UpdateCollisionData();

	/**
	 * @brief Update the path of a simulation system at the given index.
	 *
	 * This method updates the path of a simulation system at the specified index.
	 * It determines the new start and end node indices for the path based on the current path's end node index, connections, and eligible connections.
	 * If there are only two connections, it chooses the other connection as the new end node index.
	 * If there are more than two connections, it calculates the angle between the current path direction and each target path direction.
	 * Eligible connections are those with an angle less than PI / 2.
	 * If there are one or more eligible connections, it randomly selects one of them as the new end node index.
	 * If there is more than one eligible connection and the node at the new start node index is blocked, the method returns without updating the path.
	 *
	 */
	void UpdatePath(const uint32 Index);

protected:

	FTrVehicleDynamics VehicleConfig;
	FTrPathFollowingConfiguration PathFollowingConfig;
	
	int NumEntities;
	TArray<FVector> Positions;
	TArray<FVector> Velocities;
	TArray<FVector> Headings;
	TArray<FVector> Goals;
	TArray<FTrVehiclePathTransform> PathTransforms;
	TArray<int> LeadingVehicleIndices;

	// todo : Use bit flags instead of bools when more than one state is available.
	TArray<bool> PathFollowingStates;

#if !UE_BUILD_SHIPPING
	TArray<FColor> DebugColors;
#endif

	/**
	 * @brief An array of FRpSpatialGraphNode objects representing the spatial graph nodes.
	 *
	 * This variable stores all the spatial graph nodes used in the simulation system.
	 * Each node in the array represents a location or position in the spatial graph.
	 * The nodes are used to define the connectivity and relationships between different locations in the graph.
	 */
	TArray<FRpSpatialGraphNode> Nodes;
	
	FTrIntersectionManager IntersectionManager;
	FRpImplicitGrid ImplicitGrid;

private:

	float TickRate;
	FTimerHandle IntersectionTimerHandle;
	FTimerHandle AmberTimerHandle;
};
