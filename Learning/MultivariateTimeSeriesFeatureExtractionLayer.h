#pragma once
#include "TimeData.h"
#include "TimeSeriesLayer.h"

namespace rsdn
{
	namespace learning
	{
		namespace timeseries
		{
			class MultivariateTimeSeriesFeaturePacket;
			typedef std::shared_ptr<MultivariateTimeSeriesFeaturePacket> MultivariateTimeSeriesFeaturePacketPtr;

			class LEARNING_API MultivariateTimeSeriesFeatureExtractionLayerOperator : public rsdn::layer::Operator
			{
			public:
				MultivariateTimeSeriesFeatureExtractionLayerOperator();

				virtual ~MultivariateTimeSeriesFeatureExtractionLayerOperator();
				virtual ResultPtr Run(TimeSeriesDataPacketPtr indata, data::ParameterPacketPtr inparam, _type intype, data::ParameterPacketPtr param, MultivariateTimeSeriesFeaturePacketPtr outdata, std::atomic<bool>& cancel);

				virtual ResultPtr RunBatch(int batch, TimeSeriesDataPacketPtr indata, data::ParameterPacketPtr inparam, _type intype, data::ParameterPacketPtr param, MultivariateTimeSeriesFeaturePacketPtr outdata, std::atomic<bool>& cancel);

				REFLECT_CLASS(MultivariateTimeSeriesFeatureExtractionLayerOperator)
			};
			typedef std::shared_ptr<MultivariateTimeSeriesFeatureExtractionLayerOperator> MultivariateTimeSeriesFeatureExtractionLayerOperatorPtr;

			class LEARNING_API MultivariateTimeSeriesFeatureExtractionLayer : public TimeSeriesLayer
			{
			private:
				bool usedata;
				bool batchmode;
				int curbatch;
				_type intype;
				std::weak_ptr<TimeSeriesDataPacket> inpacket;
				std::weak_ptr<ParameterPacket> inparams;
				MultivariateTimeSeriesFeaturePacketPtr outpacket;
				std::vector<MultivariateTimeSeriesFeatureExtractionLayerOperatorPtr> operators;
				data::ParameterPacketPtr params;
			public:
				MultivariateTimeSeriesFeatureExtractionLayer();
				~MultivariateTimeSeriesFeatureExtractionLayer();

			protected:
				std::unique_ptr<std::future<ResultPtr>> RunAsync(std::atomic<bool>& cancel) override final;

				ResultPtr IsSupportConnectFrom(_type prev) override final;
				ResultPtr IsSupportConnectTo(_type next) override final;
				ResultPtr ReadyCore() override final;
				ResultPtr ConfigCore(data::ParameterPacketPtr param) override final;
				ResultPtr InCore(data::DataPacketPtr data, data::ParameterPacketPtr prevparameter, _type prev) override final;
				ResultPtr OutCore(data::DataPacketPtr& data, data::ParameterPacketPtr& parameter, _type next) override final;

				ResultPtr AddOperator(OperatorPtr op) override final;

				ResultPtr ConfigBatch(unsigned int batch) override final;


				REFLECT_CLASS(MultivariateTimeSeriesFeatureExtractionLayer)
			};
		}
	}
}