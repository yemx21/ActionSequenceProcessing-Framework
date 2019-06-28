#pragma once
#include "STRUCT_Config.h"

namespace rsdn
{
	namespace learning
	{
		namespace timeseries
		{
			class ActionDataPacket;
			typedef std::shared_ptr<ActionDataPacket> ActionDataPacketPtr;

			class ActionRepresentationDataPacket;
			typedef std::shared_ptr<ActionRepresentationDataPacket> ActionRepresentationDataPacketPtr;

			class STRUCT_API ActionRepresentationLayerOperator : public rsdn::layer::Operator
			{
			public:
				ActionRepresentationLayerOperator();
				virtual ~ActionRepresentationLayerOperator();
				virtual ResultPtr Run(ActionDataPacketPtr indata, data::ParameterPacketPtr inparam, _type intype, data::ParameterPacketPtr param, ActionRepresentationDataPacketPtr outdata, std::atomic<bool>& cancel);

				virtual unsigned int GetBatchCount();
				virtual ResultPtr PrepareBatchAsync(ActionDataPacketPtr indata, data::ParameterPacketPtr inparam, _type intype, data::ParameterPacketPtr param, ActionRepresentationDataPacketPtr outdata, std::atomic<bool>& cancel);
				virtual ResultPtr RunBatchAsync(ActionDataPacketPtr indata, data::ParameterPacketPtr inparam, _type intype, data::ParameterPacketPtr param, ActionRepresentationDataPacketPtr outdata, std::atomic<bool>& cancel, unsigned int batch);

				REFLECT_CLASS(ActionRepresentationLayerOperator)
			};
			typedef std::shared_ptr<ActionRepresentationLayerOperator> ActionRepresentationLayerOperatorPtr;

			class STRUCT_API ActionRepresentationLayer : public TimeSeriesLayer
			{
			private:
				_type intype;
				int tmpbatch;
				std::weak_ptr<ActionDataPacket> inpacket;
				std::weak_ptr<ParameterPacket> inparams;
				ActionRepresentationDataPacketPtr outpacket;
				std::vector<ActionRepresentationLayerOperatorPtr> operators;
				data::ParameterPacketPtr params;
			public:
				ActionRepresentationLayer();
				~ActionRepresentationLayer();

			protected:
				std::unique_ptr<std::future<ResultPtr>> RunAsync(std::atomic<bool>& cancel) override final;

				unsigned int GetBatchCount() override final;
				std::unique_ptr<std::future<ResultPtr>> PrepareBatchAsync(std::atomic<bool>& cancel) override final;
				std::unique_ptr<std::future<ResultPtr>> RunBatchAsync(std::atomic<bool>& cancel, unsigned int batch) override final;

				ResultPtr IsSupportConnectFrom(_type prev) override final;
				ResultPtr IsSupportConnectTo(_type next) override final;
				ResultPtr ReadyCore() override final;
				ResultPtr ConfigCore(data::ParameterPacketPtr param) override final;
				ResultPtr InCore(data::DataPacketPtr data, data::ParameterPacketPtr prevparameter, _type prev) override final;
				ResultPtr OutCore(data::DataPacketPtr& data, data::ParameterPacketPtr& parameter, _type next) override final;

				ResultPtr AddOperator(OperatorPtr op) override final;

				REFLECT_CLASS(ActionRepresentationLayer)
			};
		}
	}
}
