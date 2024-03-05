#include "FTrIntersectionManager.h"

void FTrIntersectionManager::Initialize(const TArray<FTrIntersection>& NewIntersections)
{
	Intersections = NewIntersections;
	PreviouslyUnblockedNodes.Init(0, Intersections.Num());
	for(const FTrIntersection& Intersection : NewIntersections)
	{
		for(const uint32 Node : Intersection.Nodes)
		{
			IntersectionNodes.Add(Node);
		}
	}
		
	SwitchToGreen();
}

bool FTrIntersectionManager::IsNodeBlocked(const uint32 NodeIndex) const
{
	if(IntersectionNodes.Contains(NodeIndex))
	{
		return !UnblockedNodes.Contains(NodeIndex);
	}

	return false;
}

void FTrIntersectionManager::SwitchToGreen()
{
	UnblockedNodes.Empty();
	for(int Index = 0; Index < Intersections.Num(); ++Index)
	{
		const FTrIntersection& Intersection = Intersections[Index];
		uint32 PreviousNode = PreviouslyUnblockedNodes[Index];
		uint32 NextNode = (PreviousNode + 1) % Intersection.Nodes.Num();
		PreviouslyUnblockedNodes[Index] = NextNode;
		UnblockedNodes.Add(Intersection.Nodes[NextNode]);
	}
}

void FTrIntersectionManager::SwitchToAmber()
{
	UnblockedNodes.Empty();
}
