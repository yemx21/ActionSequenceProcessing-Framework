#include "SkeletonRepresentationDataPacket.h"
using namespace rsdn;
using namespace rsdn::learning::timeseries;

SkeletonRepresentationSequenceDataSection::SkeletonRepresentationSequenceDataSection()
{
	Sequences = std::make_shared<SkeletonSequenceCollection>();
}

GaitCycleChannelDataSection::GaitCycleChannelDataSection()
{
	Channels = std::make_shared<GaitCycleChannelCollection>();
}

SkeletonRepresentationDataPacket::SkeletonRepresentationDataPacket()
{
	SequenceSection = std::make_shared<SkeletonRepresentationSequenceDataSection>();
	CycleSection = std::make_shared<GaitCycleChannelDataSection>();
	SampleSection = std::make_shared<GaitCycleSampleDataSection>();
}

__int64 GaitCycleChannelDataSection::GetChildCount()
{
	return (__int64)Channels->size();
}

IDataSectionPtr GaitCycleChannelDataSection::GetChild(const __int64 index)
{
	if (index<0 || index> Channels->size() - 1) return nullptr;
	return Channels->at(index);
}

void* GaitCycleSampleDataSection::GetDataCore1(const std::wstring& key)
{
	return key.compare(L"data") == 0 ? Samples.get() : nullptr;
}

GaitCycleSampleDataSection::GaitCycleSampleDataSection()
{
	Samples = std::make_unique<SampleChunk>();
}