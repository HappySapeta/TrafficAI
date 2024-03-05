#pragma once
#include "TrafficAI/Utility/TrSpatialGraphComponent.h"



// The FTrIntersectionManager class manages a set of intersections in a traffic simulation system.
class FTrIntersectionManager
{
public:
	
	// Initializes the FTrIntersectionManager with new intersections.
	void Initialize(const TArray<FTrIntersection>& NewIntersections);

	// This method checks if the specified node is blocked or not.
	bool IsNodeBlocked(const uint32 NodeIndex) const;

	/**
	 * @brief Switches the traffic signal to the green state, allowing traffic to move through the intersections.
	 *
	 * This method updates the traffic signal state of the intersections to allow vehicles to move through them.
	 * It determines the next node in the intersection for each intersection and updates the previously unblocked nodes accordingly.
	 * The unblocked nodes are then added to the set of unblocked nodes.
	 */
	void SwitchToGreen();

	// This method is responsible for switching the traffic light at an intersection to amber, indicating that the signals are about to change.
	void SwitchToAmber();

private:
	
	/**
	 * This array stores the intersections that are managed by the FTrIntersectionManager class.
	 * Each intersection represents a set of nodes in a spatial graph.
	 * The intersections are used to manage the state of traffic signals and to determine the next node
	 * for each intersection when switching the traffic signal to the green state.
	 */
	TArray<FTrIntersection> Intersections;
	
	/**
	 * This array stores the indices of nodes that make up the intersections in the simulation.
	 * Each element represents a node index in the spatial graph that belongs to an intersection.
	 *
	 * This is mainly used to speed-up lookups.
	 */
	TArray<uint32> IntersectionNodes;
	
	/**
	 * @brief Array of previously unblocked nodes.
	 *
	 * This array stores the indices of the previously unblocked nodes from each intersection.
	 * It is used by the FTrIntersectionManager class to determine the next node in the intersection for each intersection
	 * when switching the traffic signal to the green state.
	 */
	TArray<uint32> PreviouslyUnblockedNodes;

	// Set of Nodes on the Spatial Graph that are allowed to release traffic.
	TSet<uint32> UnblockedNodes;
	
};
