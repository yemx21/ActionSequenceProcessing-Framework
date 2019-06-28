#include "ActionRepresentationDataPacket.h"
using namespace rsdn;
using namespace rsdn::learning::timeseries;

ActionChannel::ActionChannel()
{
	Kinematics = std::make_shared<Buffer>();
	Labels = std::make_shared<GenericBuffer<int>>();
	Id = std::make_shared<GenericBuffer<__int64>>();
	SubId = std::make_shared<GenericBuffer<__int64>>();
	Subject = std::make_shared<GenericBuffer<__int64>>();
}

boost::any ActionChannel::GetDataCore(const std::wstring& key)
{
	if (key.compare(L"timefeatures") == 0) return Kinematics;
	if (key.compare(L"labels") == 0) return Labels;
	if (key.compare(L"ids") == 0) return Id;
	if (key.compare(L"subids") == 0) return SubId;
	if (key.compare(L"subjects") == 0) return Subject;
	return boost::any{};
}

ActionRepresentationSequenceDataSection::ActionRepresentationSequenceDataSection()
{
	Sequences = std::make_shared<ActionSequenceCollection>();
}

ActionChannelDataSection::ActionChannelDataSection()
{
	Channels = std::make_shared<ActionChannelCollection>();
}

ActionRepresentationDataPacket::ActionRepresentationDataPacket()
{
	SequenceSection = std::make_shared<ActionRepresentationSequenceDataSection>();
	CycleSection = std::make_shared<ActionChannelDataSection>();
	SampleSection = std::make_shared<ActionSampleDataSection>();
	PairwiseSection = std::make_shared<ActionPairwiseRepresentationDataSection>();
}

__int64 ActionChannelDataSection::GetChildCount()
{
	return (__int64)Channels->size();
}

IDataSectionPtr ActionChannelDataSection::GetChild(const __int64 index)
{
	if (index<0 || index> Channels->size() - 1) return nullptr;
	return Channels->at(index);
}

void* ActionSampleDataSection::GetDataCore1(const std::wstring& key)
{
	return key.compare(L"data") == 0 ? Samples.get() : nullptr;
}

ActionSampleDataSection::ActionSampleDataSection()
{
	Samples = std::make_unique<SampleChunk>();
}

ActionPairwiseRepresentationDataSection::ActionPairwiseRepresentationDataSection()
{
	Samples = std::make_unique<SampleChunk>();
}

void* ActionPairwiseRepresentationDataSection::GetDataCore1(const std::wstring& key)
{
	return key.compare(L"samples") == 0 ? Samples.get() : nullptr;
}

boost::any ActionPairwiseRepresentationDataSection::GetDataCore(const std::wstring& key)
{
	return key.compare(L"id") == 0 ? Id : nullptr;
}
void ActionPairwiseRepresentationDataSection::SetDataCore(const std::wstring& key, const boost::any& data)
{
	if (key.compare(L"id") == 0)
	{
		Id = boost::any_cast<std::shared_ptr<int>>(data);
	}
}