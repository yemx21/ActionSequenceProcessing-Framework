#pragma once
#include "SkeletonRepresentationLayer.h"
#include "SkeletonDataPacket.h"
#include "SkeletonRepresentationDataPacket.h"
#include <concurrent_vector.h>
#include <concurrent_unordered_map.h>

namespace rsdn
{
	namespace learning
	{
		namespace timeseries
		{
			namespace operators
			{
				class GaitCycleNormalizationOperator : public SkeletonRepresentationLayerOperator
				{
				private:
					ResultPtr RunCPU(SkeletonDataPacketPtr indata, int desiredframes, int osr, int sr, SkeletonRepresentationDataPacketPtr outdata, std::atomic<bool>& cancel);
				public:
					ResultPtr Run(SkeletonDataPacketPtr indata, data::ParameterPacketPtr inparam, _type intype, data::ParameterPacketPtr param, SkeletonRepresentationDataPacketPtr outdata, std::atomic<bool>& cancel) override final;
				protected:
					data::ParameterPacketPtr params;
					ResultPtr ConfigCore(data::ParameterPacketPtr param) override;

					REFLECT_CLASS(GaitCycleNormalizationOperator)
				};
			}
		}
	}
}

