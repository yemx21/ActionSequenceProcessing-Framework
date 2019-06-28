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
				class PeriodicClusteringOperator : public MultivariateTimeSeriesFeatureExtractionLayerOperator
				{
				private:
					ResultPtr RunCPU(TimeSeriesDataPacketPtr indata, data::ParameterPacketPtr inparam, bool uselabels, std::shared_ptr<GenericBuffer<int>> reflabels, int label_distfun, MultivariateTimeSeriesFeaturePacketPtr outdata, std::atomic<bool>& cancel);
					ResultPtr RunBatchCPU(int batch, TimeSeriesDataPacketPtr indata, data::ParameterPacketPtr inparam, bool uselabels, std::shared_ptr<GenericBuffer<int>> reflabels, int label_distfun, MultivariateTimeSeriesFeaturePacketPtr outdata, std::atomic<bool>& cancel);

				private:
					ResultPtr RunChannelCPU(TimeSeriesDataPacketPtr indata, data::ParameterPacketPtr inparam, int channel, bool uselabels, std::shared_ptr<GenericBuffer<int>> reflabels, int label_distfun, BufferPtr timefeatures, BufferPtr times, std::shared_ptr<GenericBuffer<int>> labels, MultivariateTimeSeriesFeaturePacketPtr outdata, std::atomic<bool>& cancel);
					ResultPtr RunChannelBatchCPU(int batch, TimeSeriesDataPacketPtr indata, data::ParameterPacketPtr inparam, int channel, bool uselabels, std::shared_ptr<GenericBuffer<int>> reflabels, int label_distfun, BufferPtr timefeatures, BufferPtr times, std::shared_ptr<GenericBuffer<int>> labels, MultivariateTimeSeriesFeaturePacketPtr outdata, std::atomic<bool>& cancel);

				protected:
					data::ParameterPacketPtr params;
					ResultPtr ConfigCore(data::ParameterPacketPtr param) override;
				
				public:
					ResultPtr Run(TimeSeriesDataPacketPtr indata, data::ParameterPacketPtr inparam, _type intype, data::ParameterPacketPtr param, MultivariateTimeSeriesFeaturePacketPtr outdata, std::atomic<bool>& cancel) override final;

					ResultPtr RunBatch(int batch, TimeSeriesDataPacketPtr indata, data::ParameterPacketPtr inparam, _type intype, data::ParameterPacketPtr param, MultivariateTimeSeriesFeaturePacketPtr outdata, std::atomic<bool>& cancel) override final;

					REFLECT_CLASS(PeriodicClusteringOperator)
				};
			}
		}
	}
}

