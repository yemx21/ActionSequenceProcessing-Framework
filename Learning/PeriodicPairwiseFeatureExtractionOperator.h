#pragma once
#include "TimeData.h"
#include "MultivariateTimeSeriesFeatureExtractionLayer.h"

namespace rsdn
{
	namespace learning
	{
		namespace timeseries
		{
			namespace operators
			{
				class PeriodicPairwiseFeatureChannelSection : public IDataSection
				{
				public:
					std::shared_ptr<GenericBuffer<std::pair<datatype, datatype>>> Pairs;
					PeriodicPairwiseFeatureChannelSection();
				protected:
					boost::any GetDataCore(const std::wstring& key) override;
				};
				typedef std::shared_ptr<PeriodicPairwiseFeatureChannelSection> PeriodicPairwiseFeatureChannelSectionPtr;
				typedef std::vector<PeriodicPairwiseFeatureChannelSectionPtr> PeriodicPairwiseFeatureChannelCollection;
				typedef std::shared_ptr<PeriodicPairwiseFeatureChannelCollection> PeriodicPairwiseFeatureChannelCollectionPtr;

				class PeriodicPairwiseFeatureSection : public IDataSection
				{
				public:
					PeriodicPairwiseFeatureChannelCollectionPtr Channels;
					PeriodicPairwiseFeatureSection();

					void WriteToBSDBWriter(BSDBWriterPtr writer);
				protected:
					boost::any GetDataCore(const std::wstring& key) override;
				};
				typedef std::shared_ptr<PeriodicPairwiseFeatureSection> PeriodicPairwiseFeatureSectionPtr;

				class PeriodicPairwiseFeatureExtractionOperator : public MultivariateTimeSeriesFeatureExtractionLayerOperator
				{
				private:
					ResultPtr RunBatchCPU(int batch, TimeSeriesDataPacketPtr indata, data::ParameterPacketPtr inparam, bool uselabels, std::shared_ptr<GenericBuffer<int>> reflabels, int label_distfun, MultivariateTimeSeriesFeaturePacketPtr outdata, std::atomic<bool>& cancel);

					ResultPtr RunCPU(TimeSeriesDataPacketPtr indata, data::ParameterPacketPtr inparam, bool uselabels, std::shared_ptr<GenericBuffer<int>> reflabels, int label_distfun, MultivariateTimeSeriesFeaturePacketPtr outdata, std::atomic<bool>& cancel);
				protected:
					data::ParameterPacketPtr params;
					ResultPtr ConfigCore(data::ParameterPacketPtr param) override;
				public:
					ResultPtr Run(TimeSeriesDataPacketPtr indata, data::ParameterPacketPtr inparam, _type intype, data::ParameterPacketPtr param, MultivariateTimeSeriesFeaturePacketPtr outdata, std::atomic<bool>& cancel) override final;

					ResultPtr RunBatch(int batch, TimeSeriesDataPacketPtr indata, data::ParameterPacketPtr inparam, _type intype, data::ParameterPacketPtr param, MultivariateTimeSeriesFeaturePacketPtr outdata, std::atomic<bool>& cancel) override final;


					REFLECT_CLASS(PeriodicPairwiseFeatureExtractionOperator)
				};
			}
		}
	}
}
