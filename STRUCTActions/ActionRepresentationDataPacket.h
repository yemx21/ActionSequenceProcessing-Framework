#pragma once
#include "ActionRepresentationLayer.h"
#include "ActionDataPacket.h"

namespace rsdn
{
	namespace learning
	{
		namespace timeseries
		{
			class STRUCT_API ActionChannel : public IDataSection
			{
			public:
				BufferPtr Kinematics;
				std::shared_ptr<GenericBuffer<int>> Labels;
				std::shared_ptr<GenericBuffer<__int64>> Id;
				std::shared_ptr<GenericBuffer<__int64>> SubId;
				std::shared_ptr<GenericBuffer<__int64>> Subject;
				ActionChannel();

			protected:
				boost::any GetDataCore(const std::wstring& key) override;
			};
			typedef std::shared_ptr<ActionChannel> ActionChannelPtr;

			typedef std::vector<ActionDataSequencePtr> ActionSequenceCollection;
			typedef std::shared_ptr<ActionSequenceCollection> ActionSequenceCollectionPtr;

			typedef std::vector<ActionChannelPtr> ActionChannelCollection;
			typedef std::shared_ptr<ActionChannelCollection> ActionChannelCollectionPtr;

			class STRUCT_API ActionRepresentationSequenceDataSection : public IDataSection
			{
			public:
				ActionSequenceCollectionPtr Sequences;
				ActionRepresentationSequenceDataSection();
			};
			typedef std::shared_ptr<ActionRepresentationSequenceDataSection> ActionRepresentationSequenceDataSectionPtr;

			class STRUCT_API ActionChannelDataSection : public IDataSection
			{
			public:
				ActionChannelCollectionPtr Channels;
				ActionChannelDataSection();

			public:
				__int64 GetChildCount() override;
				IDataSectionPtr GetChild(const __int64 key) override;
			};
			typedef std::shared_ptr<ActionChannelDataSection> ActionChannelDataSectionPtr;

			class STRUCT_API ActionSampleDataSection : public IDataSection
			{
			public:
				std::unique_ptr<SampleChunk> Samples;
			protected:
				void* GetDataCore1(const std::wstring& key) override;
			public:
				ActionSampleDataSection();
			};
			typedef std::shared_ptr<ActionSampleDataSection> ActionSampleDataSectionPtr;

			class STRUCT_API ActionPairwiseRepresentationDataSection : public IDataSection
			{
			public:
				std::shared_ptr<int> Id;
				std::unique_ptr<SampleChunk> Samples;
			protected:
				void* GetDataCore1(const std::wstring& key) override;
				boost::any GetDataCore(const std::wstring& key) override;
				void SetDataCore(const std::wstring& key, const boost::any& data) override;
			public:
				ActionPairwiseRepresentationDataSection();
			};
			typedef std::shared_ptr<ActionPairwiseRepresentationDataSection> ActionPairwiseRepresentationDataSectionPtr;

			class STRUCT_API ActionRepresentationDataPacket : public TimeSeriesDataPacket
			{
			public:
				ActionRepresentationSequenceDataSectionPtr SequenceSection;
				ActionChannelDataSectionPtr CycleSection;
				ActionSampleDataSectionPtr SampleSection;
				ActionPairwiseRepresentationDataSectionPtr PairwiseSection;

				ActionRepresentationDataPacket();

				REFLECT_CLASS(ActionRepresentationDataPacket)
			};
		}
	}
}