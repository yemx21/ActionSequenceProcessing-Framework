#include "MultivariateTimeSeriesFeaturePacket.h"
using namespace rsdn;
using namespace rsdn::learning;
using namespace rsdn::learning::timeseries;

TimeSeriesChannelSection::TimeSeriesChannelSection()
{
	Features = std::make_shared<Buffer>();
	Times = std::make_shared<Buffer>();
	Labels = std::make_shared<GenericBuffer<int>>();
}

boost::any TimeSeriesChannelSection::GetDataCore(const std::wstring& key)
{
	if (key.compare(L"features") == 0) return Features;
	if (key.compare(L"times") == 0) return Times;
	if (key.compare(L"labels") == 0) return Labels;
	return nullptr;
}

MultivariateTimeSeriesSection::MultivariateTimeSeriesSection()
{
	Channels = std::make_shared<TimeSeriesChannelSectionCollection>();
}

__int64 MultivariateTimeSeriesSection::GetChildCount()
{
	return Channels->size();
}

IDataSectionPtr MultivariateTimeSeriesSection::GetChild(const __int64 index)
{
	if (index<0 || index> (__int64) Channels->size() - 1) return nullptr;
	return Channels->at(index);
}

MultivariateTimeSeriesFeaturePacket::MultivariateTimeSeriesFeaturePacket()
{
	TemplateSection = std::make_shared<MultivariateTimeSeriesSection>();
}