#pragma once
#include "Layer.h"
#include <map>
#include <set>
using namespace rsdn::data;
using namespace rsdn::layer;

namespace rsdn
{
	class Node;
	typedef std::shared_ptr<Node> NodePtr;

	class Graph;
	typedef std::shared_ptr<Graph> GraphPtr;

	class CORE_API Graph
	{
	private:
		std::map<int64_t, LayerPtr> pool;
		std::map<int64_t, std::set<int64_t>> children;
		std::map<int64_t, std::set<int64_t>> parents;

		inline bool GetLevelForward(int64_t id, std::map<int64_t, Optional<int>>& levels);
		inline bool GetLevelBackward(int64_t id, std::map<int64_t, Optional<int>>& levels);
		inline bool RearrangeLevels(std::map<int64_t, Optional<int>>& levels);
	public:
		bool AddLayer(LayerPtr layer);
		void RemoveLayer(LayerPtr layer);

		bool ConnectFromTo(LayerPtr layer1, LayerPtr layer2);
		void DisconnectFromTo(LayerPtr layer1, LayerPtr layer2);

		bool IsConnected(LayerPtr layer1, LayerPtr layer2);

		ResultPtr AddOperator(LayerPtr layer, OperatorPtr op);

		AsyncResultPtr RunAsync(bool standalonefirst = true);
		
		static GraphPtr LoadFromFile(const std::wstring& path, ResultPtr& ret, const std::locale& loc);
	};
	
}