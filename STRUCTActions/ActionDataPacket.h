#pragma once
#include "STRUCTDataLayer.h"
namespace rsdn
{
	namespace learning
	{
		namespace timeseries
		{
			class STRUCT_API ActionDataSequence
			{
			public:
				std::shared_ptr<GenericBuffer<Point3>> Joints;
				BufferPtr KinematicParameters;
				unsigned __int64 FrameCount;
				std::shared_ptr<GenericBuffer<int>> Labels;
				int ActionLabel;
				int Subject;
				int ViewId;
				__int64 Id;
				ActionDataSequence();
			};
			typedef std::shared_ptr<ActionDataSequence> ActionDataSequencePtr;

			typedef std::vector<ActionDataSequencePtr> ActionSequenceCollection;
			typedef std::shared_ptr<ActionSequenceCollection> ActionSequenceCollectionPtr;

			class STRUCT_API ActionDataPacket : public TimeSeriesDataPacket
			{
			public:
				ActionSequenceCollectionPtr Sequences;

				ActionDataPacket();

				REFLECT_CLASS(ActionDataPacket)
			};
		}
	}
}