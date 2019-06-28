#pragma once
#include "Learning_Config.h"


namespace rsdn
{
	namespace learning
	{
		namespace machines
		{
			namespace supervised
			{
				class LEARNING_API DecisionUnitL1
				{
				public:
					static void Train(const SampleChunk* data, int splitoffset, const std::vector<sizetype>& indices, const std::vector<sizetype>& featindices, std::vector<datatype>& factors, datatype& c, datatype& qual, int batchstablity = 50, datatype learningRate = 0.02, int maxIter = 2000, datatype convergence = 0.01, datatype regularizationRate = 0.0001);
				};
			}
		}
	}
}
