#pragma once
#include "Learning_Config.h"
#include <tuple>
#include <unordered_set>
#include <unordered_map>
#include <concurrent_vector.h>
#include <concurrent_unordered_map.h>

namespace rsdn
{
	namespace learning
	{
		namespace timeseries
		{
			class LEARNING_API DTWCluster
			{
			public:
				static void ClusteringLP(const datatype* data, const int* labels, int featnum, int num, float lamda, const int* reflabels, int reflabelnum, float labeldistancebaseterm, int sparse, BufferPtr outdata, std::shared_ptr<GenericBuffer<int>> outlabels);

				static void ClusteringS(const datatype* data, const int* labels, int featnum, int num, int sparse, BufferPtr outdata, std::shared_ptr<GenericBuffer<int>> outlabels, int dbscan_order=-1);

				class LEARNING_API DTWCache
				{
				public:
					int Length;
					Table<datatype>* Distances;
					Table<datatype>* PathCosts;
					Table<int>* Step1;
					Table<int>* Step2;

					DTWCache(int len);
					~DTWCache();
					void Reset();
				};

				static datatype DTWCost(DTWCache* cache, const datatype* data1, const int* labels1, const datatype* data2, const int* labels2, const std::map<int, std::map<int, float>>& labeldistances);

				static datatype DTWPath(DTWCache* cache, const datatype* data1, const int* labels1, const datatype* data2, const int* labels2, const std::map<int, std::map<int, float>>& labeldistances, std::vector<std::tuple<int, int>>& paths);

				static datatype DTWPath2(DTWCache* cache, const datatype* data1, const int* labels1, const std::vector<datatype>& sch2, const std::vector<std::map<int, float>>& schlab2, const std::map<int, std::map<int, float>>& labeldistances, float lamda, std::vector<std::tuple<int, int>>& paths);

				static datatype DTWPath0(DTWCache* cache, const datatype* data1, const std::vector<datatype>& sch2, float lamda, std::vector<std::tuple<int, int>>& paths);
				
				static datatype DTWPath0(DTWCache* cache, const datatype* data1, const std::vector<datatype>& sch2, std::vector<std::tuple<int, int>>& paths);

				static datatype DTWCost0(DTWCache* cache, const datatype* data1, const datatype* data2);


				static void DBScan(DTWCache* cache, const datatype* data, const int* labels, int featnum, int num, int order, int sparse, const std::map<int, std::map<int, float>>& labeldistances, std::unordered_map<int, std::vector<int>>& clusters);

				static void DBA(DTWCache* cache, const datatype* data, const int* labels, int featnum, const std::vector<int>& indexs, const std::map<int, std::map<int, float>>& labeldistances, float lamda, size_t maxiterations, BufferPtr outdata, std::shared_ptr<GenericBuffer<int>> outlabels);
			
				static void DBScan(DTWCache* cache, const datatype* data, const std::vector<int>& indices, int featnum, int num, int order, int sparse, std::unordered_map<int, std::vector<int>>& clusters);

				static void DBA(DTWCache* cache, const datatype* data, int featnum, const std::vector<int>& indexs, size_t maxiterations, BufferPtr outdata);

			};
		}
	}
}