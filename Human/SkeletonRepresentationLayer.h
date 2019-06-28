#pragma once
#include "Human_Config.h"

namespace rsdn
{
	namespace learning
	{
		namespace timeseries
		{
			class SkeletonDataPacket;
			typedef std::shared_ptr<SkeletonDataPacket> SkeletonDataPacketPtr;

			class SkeletonRepresentationDataPacket;
			typedef std::shared_ptr<SkeletonRepresentationDataPacket> SkeletonRepresentationDataPacketPtr;

			class HUMAN_API SkeletonRepresentationLayerOperator: public rsdn::layer::Operator
			{
			public:				
				SkeletonRepresentationLayerOperator();
				virtual ~SkeletonRepresentationLayerOperator();
				virtual ResultPtr Run(SkeletonDataPacketPtr indata, data::ParameterPacketPtr inparam, _type intype, data::ParameterPacketPtr param, SkeletonRepresentationDataPacketPtr outdata, std::atomic<bool>& cancel);

				REFLECT_CLASS(SkeletonRepresentationLayerOperator)
			};
			typedef std::shared_ptr<SkeletonRepresentationLayerOperator> SkeletonRepresentationLayerOperatorPtr;

			class HUMAN_API SkeletonRepresentationLayer : public TimeSeriesLayer
			{
			private:
				_type intype;
				std::weak_ptr<SkeletonDataPacket> inpacket;
				std::weak_ptr<ParameterPacket> inparams;
				SkeletonRepresentationDataPacketPtr outpacket;
				std::vector<SkeletonRepresentationLayerOperatorPtr> operators;
				data::ParameterPacketPtr params;
			public:
				SkeletonRepresentationLayer();
				~SkeletonRepresentationLayer();

			protected:
				std::unique_ptr<std::future<ResultPtr>> RunAsync(std::atomic<bool>& cancel) override final;

				ResultPtr IsSupportConnectFrom(_type prev) override final;
				ResultPtr IsSupportConnectTo(_type next) override final;
				ResultPtr ReadyCore() override final;
				ResultPtr ConfigCore(data::ParameterPacketPtr param) override final;
				ResultPtr InCore(data::DataPacketPtr data, data::ParameterPacketPtr prevparameter, _type prev) override final;
				ResultPtr OutCore(data::DataPacketPtr& data, data::ParameterPacketPtr& parameter, _type next) override final;

				ResultPtr AddOperator(OperatorPtr op) override final;

				REFLECT_CLASS(SkeletonRepresentationLayer)
			};
		}
	}
}