#pragma once
#include "Graph.h"

namespace rsdn
{
	class Node
	{
	public:
		bool Finished;
		LayerPtr Item;
		int BatchCount;
		int CurrentBatch;
		std::vector<NodePtr> Previous;
		std::vector<NodePtr> Next;
		Node();
	};
}

