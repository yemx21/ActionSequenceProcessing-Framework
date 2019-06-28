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
				class RandomForestLayerDataPacket;
				typedef std::shared_ptr<RandomForestLayerDataPacket> RandomForestLayerDataPacketPtr;

				class LEARNING_API RandomForestLayer : public Layer
				{
				private:
					_type intype;
					std::weak_ptr<DataPacket> inpacket;
					std::weak_ptr<ParameterPacket> inparams;
					RandomForestLayerDataPacketPtr outpacket;
					data::ParameterPacketPtr params;
				public:
					RandomForestLayer();
					~RandomForestLayer();

				protected:
					std::unique_ptr<std::future<ResultPtr>> RunAsync(std::atomic<bool>& cancel) override final;

					ResultPtr IsSupportConnectFrom(_type prev) override final;
					ResultPtr IsSupportConnectTo(_type next) override final;
					ResultPtr ReadyCore() override final;
					ResultPtr ConfigCore(data::ParameterPacketPtr param) override final;
					ResultPtr InCore(data::DataPacketPtr data, data::ParameterPacketPtr prevparameter, _type prev) override final;
					ResultPtr OutCore(data::DataPacketPtr& data, data::ParameterPacketPtr& parameter, _type next) override final;

					REFLECT_CLASS(RandomForestLayer)
				};
			}
		}
	}
}
