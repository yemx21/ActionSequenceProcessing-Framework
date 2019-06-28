#pragma once
#include "STRUCT_Config.h"
namespace rsdn
{
	namespace learning
	{
		namespace timeseries
		{
			#define STRUCT_JOINTCOUNT 25

			class ActionDataPacket;
			typedef std::shared_ptr<ActionDataPacket> ActionDataPacketPtr;

			class STRUCT_API STRUCTDataLayer : public DataLayer
			{
			private:
				ActionDataPacketPtr packet;
			public:
				STRUCTDataLayer();
				~STRUCTDataLayer();

				bool Open(const std::wstring& path) override;

			protected:
				std::unique_ptr<std::future<ResultPtr>> RunAsync(std::atomic<bool>& cancel) override final;

				ResultPtr ReadyCore() override final;
				ResultPtr IsSupportConnectTo(_type next) override final;
				ResultPtr OutCore(data::DataPacketPtr& data, data::ParameterPacketPtr& parameter, _type next) override final;

				REFLECT_CLASS(STRUCTDataLayer)
			};
		}
	}
}
