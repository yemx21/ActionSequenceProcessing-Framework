#pragma once
#include "TimeData.h"
#include "MultivariateTimeSeriesFeatureExtractionLayer.h"
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
				class PairwiseFieldOperator : public MultivariateTimeSeriesFeatureExtractionLayerOperator
				{
				private:
					ResultPtr RunCPU(TimeSeriesDataPacketPtr indata, data::ParameterPacketPtr inparam, MultivariateTimeSeriesFeaturePacketPtr outdata, std::atomic<bool>& cancel);
					ResultPtr RunBatchCPU(int batch, TimeSeriesDataPacketPtr indata, data::ParameterPacketPtr inparam, MultivariateTimeSeriesFeaturePacketPtr outdata, std::atomic<bool>& cancel);

				protected:
					data::ParameterPacketPtr params;
					ResultPtr ConfigCore(data::ParameterPacketPtr param) override;

				public:
					ResultPtr Run(TimeSeriesDataPacketPtr indata, data::ParameterPacketPtr inparam, _type intype, data::ParameterPacketPtr param, MultivariateTimeSeriesFeaturePacketPtr outdata, std::atomic<bool>& cancel) override final;
					ResultPtr RunBatch(int batch, TimeSeriesDataPacketPtr indata, data::ParameterPacketPtr inparam, _type intype, data::ParameterPacketPtr param, MultivariateTimeSeriesFeaturePacketPtr outdata, std::atomic<bool>& cancel) override final;

					REFLECT_CLASS(PairwiseFieldOperator)
				};
			}
		}
	}
}


