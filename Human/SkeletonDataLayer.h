#pragma once
#include "Human_Config.h"
namespace rsdn
{
	namespace learning
	{
		namespace timeseries
		{
			/*
			SKEL Data Layout (BSDB database)

			Header: SKEL(%version[int32])
			Items: [skeletonmodel:(%jointcount[int32]){(%jointid[int32])(%joint_name[str])}(%rootcount[int32])(%rootid[int32])(%groupcount[int32]){group:(%parentid[int32])(%childcount[int32]){(%childid[int32])})}]
			Data: {sequence:[(%sequence_id[int64])(%subject_id[int64])(%jointcount[int32]){(%jointid[int32])}(%has_floor[bool])(floor:(%a[float])(%b[float])(%c[float])(%d[float]))(%framecount[uint64]){(%timestamp[datatype:ms])}{jointchannel:{frame:[joint:(%x[float])(%y[float])(%z[float])]}}(%has_frame_label[bool]){%frame_label[int32]}(%has_event[bool])(%eventcount[int32]){(%event_time[datatype[ms]])}{(%event_id[int32])}}]}

			*/

			class SkeletonDataPacket;
			typedef std::shared_ptr<SkeletonDataPacket> SkeletonDataPacketPtr;

			class HUMAN_API SkeletonDataLayer: public DataLayer
			{
			private:
				SkeletonDataPacketPtr packet;
			public:
				SkeletonDataLayer();
				~SkeletonDataLayer();

				bool Open(const std::wstring& path) override;

			protected:
				std::unique_ptr<std::future<ResultPtr>> RunAsync(std::atomic<bool>& cancel) override final;

				ResultPtr ReadyCore() override final;
				ResultPtr IsSupportConnectTo(_type next) override final;
				ResultPtr OutCore(data::DataPacketPtr& data, data::ParameterPacketPtr& parameter, _type next) override final;

				REFLECT_CLASS(SkeletonDataLayer)
			};
		}
	}
}
