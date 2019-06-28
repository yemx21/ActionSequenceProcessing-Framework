#include "SampleChunkLayer.h"
#include <filesystem>
#include "Converter.h"
using namespace rsdn;
using namespace rsdn::layer;
using namespace rsdn::data;

SampleChunkDataPacket::SampleChunkDataPacket()
{
	Samples = std::make_shared<SampleChunkDataSection>();
	SetSection(L"samples", Samples);
}

void* SampleChunkDataSection::GetDataCore1(const std::wstring& key)
{
	return key.compare(L"data") == 0 ? Samples.get() : nullptr;
}

SampleChunkDataSection::SampleChunkDataSection()
{
	Samples = std::make_unique<SampleChunk>();
}


SampleChunkLayer::SampleChunkLayer()
{
	outpacket = std::make_shared<SampleChunkDataPacket>();
}

SampleChunkLayer::~SampleChunkLayer()
{

}

bool SampleChunkLayer::Open(const std::wstring& path)
{
	std::experimental::filesystem::path pathinfo = path;
	if (!std::experimental::filesystem::exists(pathinfo)) return false;
	if (pathinfo.extension().compare(".samples") != 0) return false;
	inpath = path;
	return true;
}

std::unique_ptr<std::future<ResultPtr>> SampleChunkLayer::RunAsync(std::atomic<bool>& cancel)
{
	return std::make_unique<std::future<ResultPtr>>(std::async(std::launch::deferred, [&cancel, &datapacket = outpacket, &datapath = inpath]()
	{
		ResultPtr ret = std::make_shared<Result>(false);
		try
		{
			ret->State = datapacket->Samples->Samples->Load(datapath);
		}
		catch (const std::exception& excep)
		{
			ret->Error = Converter::Convert(excep.what());
		}
		return ret;
	}));
}

ResultPtr SampleChunkLayer::IsSupportConnectTo(_type next)
{
	bool ret = next->IsBaseOf(_typeof(Layer));
	return ret ? std::make_shared<Result>(true) : std::make_shared<Result>(false, L"next layer is not a Layer");
}

ResultPtr SampleChunkLayer::ReadyCore()
{
	return !inpath.empty() ? std::make_shared<Result>(true) : std::make_shared<Result>(false, L"data file path is not specified");
}

ResultPtr SampleChunkLayer::OutCore(data::DataPacketPtr& data, data::ParameterPacketPtr& parameter, _type next)
{
	data = outpacket;
	parameter = nullptr;
	return std::make_shared<Result>(true);
}
