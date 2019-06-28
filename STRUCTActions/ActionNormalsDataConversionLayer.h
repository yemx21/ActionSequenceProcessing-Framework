#pragma once
#include "STRUCT_Config.h"

namespace rsdn
{
	namespace learning
	{
		namespace timeseries
		{
			class STRUCT_API ActionNormalsDataConversionLayer : public Layer
			{
			private:
				data::ParameterPacketPtr params;
			public:
				ActionNormalsDataConversionLayer();
				~ActionNormalsDataConversionLayer();

			protected:
				std::unique_ptr<std::future<ResultPtr>> RunAsync(std::atomic<bool>& cancel) override final;

				ResultPtr IsSupportConnectFrom(_type prev) override final;
				ResultPtr IsSupportConnectTo(_type next) override final;
				ResultPtr ReadyCore() override final;
				ResultPtr ConfigCore(data::ParameterPacketPtr param) override final;
				ResultPtr InCore(data::DataPacketPtr data, data::ParameterPacketPtr prevparameter, _type prev) override final;
				ResultPtr OutCore(data::DataPacketPtr& data, data::ParameterPacketPtr& parameter, _type next) override final;

				REFLECT_CLASS(ActionNormalsDataConversionLayer)
			};
		}
	}
}
