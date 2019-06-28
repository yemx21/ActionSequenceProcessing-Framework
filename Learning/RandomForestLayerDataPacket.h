#pragma once
#include "RandomForestLayer.h"

namespace rsdn
{
	namespace learning
	{
		namespace machines
		{
			namespace supervised
			{
				class RandomForestLayerDataPacket: public DataPacket
				{
				public:
					RandomForestLayerDataPacket();
					~RandomForestLayerDataPacket();

					REFLECT_CLASS(RandomForestLayerDataPacket)
				};
			}
		}
	}
}