#pragma once
#include "Learning_Config.h"

namespace rsdn
{
	namespace learning
	{
		namespace timeseries
		{
			class LEARNING_API TimeSeriesDataPacket : public DataPacket
			{

				REFLECT_CLASS(TimeSeriesDataPacket)
			};
			typedef std::shared_ptr<TimeSeriesDataPacket> TimeSeriesDataPacketPtr;
		}
	}
}