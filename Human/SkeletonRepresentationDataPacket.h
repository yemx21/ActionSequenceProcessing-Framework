#pragma once
#include "SkeletonRepresentationLayer.h"
#include "SkeletonDataPacket.h"

namespace rsdn
{
	namespace learning
	{
		namespace timeseries
		{		
			class HUMAN_API SkeletonRepresentationSequenceDataSection: public IDataSection
			{
			public:
				SkeletonSequenceCollectionPtr Sequences;
				SkeletonRepresentationSequenceDataSection();
			};
			typedef std::shared_ptr<SkeletonRepresentationSequenceDataSection> SkeletonRepresentationSequenceDataSectionPtr;

			class HUMAN_API GaitCycleChannelDataSection : public IDataSection
			{
			public:
				GaitCycleChannelCollectionPtr Channels;
				GaitCycleChannelDataSection();

			public:
				__int64 GetChildCount() override;
				IDataSectionPtr GetChild(const __int64 key) override;
			};
			typedef std::shared_ptr<GaitCycleChannelDataSection> GaitCycleChannelDataSectionPtr;

			class HUMAN_API GaitCycleSampleDataSection : public IDataSection
			{
			public:
				std::unique_ptr<SampleChunk> Samples;
			protected:
				void* GetDataCore1(const std::wstring& key) override;
			public:
				GaitCycleSampleDataSection();
			};
			typedef std::shared_ptr<GaitCycleSampleDataSection> GaitCycleSampleDataSectionPtr;

			class HUMAN_API SkeletonRepresentationDataPacket : public TimeSeriesDataPacket
			{
			public:
				SkeletonRepresentationSequenceDataSectionPtr SequenceSection;
				GaitCycleChannelDataSectionPtr CycleSection;
				GaitCycleSampleDataSectionPtr SampleSection;
				SkeletonRepresentationDataPacket();

				REFLECT_CLASS(SkeletonRepresentationDataPacket)
			};
		}
	}
}