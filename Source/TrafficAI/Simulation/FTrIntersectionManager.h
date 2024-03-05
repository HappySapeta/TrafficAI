#pragma once
#include "TrafficAI/Utility/TrSpatialGraphComponent.h"

class FTrIntersectionManager
{
public:

	void Initialize(const TArray<FTrIntersection>& NewIntersections);

	bool IsNodeBlocked(const uint32 NodeIndex) const;

	void SwitchToGreen();

	void SwitchToAmber();

private:

	TArray<FTrIntersection> Intersections;
	TArray<uint32> IntersectionNodes;
	TArray<uint32> PreviouslyUnblockedNodes;
	TSet<uint32> UnblockedNodes;
};
