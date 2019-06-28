#include "RandomForestLayer.h"
#include "RandomForestLayerDataPacket.h"
#include "RandomDecisionSea.h"
using namespace rsdn;
using namespace rsdn::learning;
using namespace rsdn::learning::machines;
using namespace rsdn::learning::machines::supervised;
using namespace rsdn::learning::machines::supervised::details;

namespace rsdn
{
	namespace learning
	{
		namespace machines
		{
			namespace supervised
			{
				namespace details
				{
					class CustomRSSelector : public IFeatureSelector
					{
					public:
						void OnSelecting(const std::vector<sizetype>& featureindexs, const std::vector<std::pair<sizetype, sizetype>>& statistics, std::vector<sizetype>& ret) override
						{
							//ret = { 1,2,3,4,5,6,7,8,9 };
							ret = featureindexs;
							return;
						}
					};
				}
			}
		}
	}
}



RandomForestLayer::RandomForestLayer()
{

}

RandomForestLayer::~RandomForestLayer()
{

}

std::unique_ptr<std::future<ResultPtr>> RandomForestLayer::RunAsync(std::atomic<bool>& cancel)
{
	return std::make_unique<std::future<ResultPtr>>(std::async(std::launch::deferred, [&cancel, &indata = inpacket, &inty = intype, &outdata = outpacket, &inparam = inparams, &param = params]()
	{
		ResultPtr ret = std::make_shared<Result>(false);
		try
		{
			DataPacketPtr inptr = indata.lock();
			if (!inptr) { throw std::exception{ "input packet is expired" }; }

			std::wstring mode;
			try
			{
				mode = param->GetItem<std::wstring>(L"mode");
			}
			catch (...)
			{
				ret->Message = L"invalid parameter: mode";
				return ret;
			}

			int job = 1;
			try
			{
				job = param->GetItem<int>(L"job");
				if (job == -1)
				{
					job = Runtime::Get().GetCpuJobCount();
				}
			}
			catch (...)
			{
			}

			if (mode.compare(L"rstrain") == 0)
			{
			#pragma region rstrain
				std::wstring modeloutput;
				try
				{
					modeloutput = param->GetItem<std::wstring>(L"output");
				}
				catch (...)
				{
					ret->Message = L"invalid parameter: output";
					return ret;
				}

				IDataSectionPtr samplepacket;
				SampleChunk* samples = nullptr;
				try
				{
					samplepacket = inptr->GetSection(L"samples");
					samples = samplepacket->GetData1<SampleChunk>(L"data");
				}
				catch (...)
				{
					ret->Message = L"no samples for training";
					return ret;
				}

				int lake = 30;
				try
				{
					lake = param->GetItem<int>(L"lake");
				}
				catch (...)
				{
					ret->Message = L"invalid parameter: lake";
					return ret;
				}

				int depth = 20;
				try
				{
					depth = param->GetItem<int>(L"depth");
				}
				catch (...)
				{
					ret->Message = L"invalid parameter: depth";
					return ret;
				}

				RandomSea<CustomRSSelector> sea;
				std::vector<sizetype> labels;
				const auto& ulabels = samples->UniqueLabels;
				for (int ul : ulabels) labels.push_back(ul);
				auto majorft = (sizetype)floor(sqrt(samples->FeatureCount) + 0.5);
				if (majorft < 1) majorft = 1;
				sizetype msc = (sizetype)((double)samples->Count * 0.01);
				if (msc > 100) msc = 100;
				if (msc < 1) msc = 1;

				std::vector<int> labs;
				for (int i = 0; i < samples->Count; i++)
				{
					labs.push_back(samples->Label[i]);
				}

				sea.Train(samples, labels, lake, depth, majorft, majorft, msc, job);
				sea.Save(modeloutput.c_str());

				Logger::Get().Report(LogLevel::Info) << L"model is saved to " << modeloutput << Logger::endl;
				#pragma endregion
			}
			else
			{
				#pragma region rstest
				std::wstring modelinput;
				try
				{
					modelinput = param->GetItem<std::wstring>(L"input");
				}
				catch (...)
				{
					ret->Message = L"invalid parameter: input";
					return ret;
				}

				IDataSectionPtr samplepacket;
				SampleChunk* samples = nullptr;
				try
				{
					samplepacket = inptr->GetSection(L"samples");
					samples = samplepacket->GetData1<SampleChunk>(L"data");
				}
				catch (...)
				{
					ret->Message = L"no samples for testing";
					return ret;
				}

				RandomSea<CustomRSSelector> sea;
				sea.Load(modelinput.c_str());
				auto predicts = sea.Predict(samples);
				
				std::map<int, int> ccs;
				std::map<int, int> ccsp;
				for (int ul = 1; ul <= 9; ul++)
				{
					ccs.insert(std::make_pair(ul, 0));
					ccsp.insert(std::make_pair(ul, 0));
				}
				for (sizetype ci = 0; ci < samples->Count; ci++)
				{
					ccs[samples->Label[ci]]++;
					ccsp[predicts[ci]]++;
				}

				sizetype corrs = 0;
				for (int i = 0; i < samples->Count; i++)
				{
					if (predicts[i] == samples->Label[i])
					{
						corrs++;
					}
				}

				Logger::Get().Report(LogLevel::Info) << corrs<< L"/" << samples->Count << " = " << std::to_wstring(corrs*100.0 /samples->Count) <<"%"  <<  Logger::endl;

				#pragma endregion
			}

			ret->State = true;
		}
		catch (const std::exception& excep)
		{
			ret->Error = Converter::Convert(excep.what());
		}
		return ret;
	}));
}

ResultPtr RandomForestLayer::IsSupportConnectFrom(_type prev)
{
	bool ret = prev->IsBaseOf(_typeof(Layer));
	return ret ? std::make_shared<Result>(true) : std::make_shared<Result>(false, L"prev layer must be a Layer");
}

ResultPtr RandomForestLayer::IsSupportConnectTo(_type next)
{
	bool ret = next->IsBaseOf(_typeof(Layer));
	return ret ? std::make_shared<Result>(true) : std::make_shared<Result>(false, L"prev layer must be a Layer");
}

ResultPtr RandomForestLayer::ReadyCore()
{
	return !inpacket.expired() ? std::make_shared<Result>(true) : std::make_shared<Result>(false, L"data in expired");
}

ResultPtr RandomForestLayer::ConfigCore(data::ParameterPacketPtr param)
{
	params = param;
	return std::make_shared<Result>(true);
}

ResultPtr RandomForestLayer::InCore(data::DataPacketPtr data, data::ParameterPacketPtr prevparameter, _type prev)
{
	if (!data->GetType()->IsBaseOf(_typeof(DataPacket))) return std::make_shared<Result>(false, L"prev data packet must be DataPacket");
	if (!data->HasSection(L"samples")) return std::make_shared<Result>(false, L"prev data packet has no samples");
	inpacket = data;
	return std::make_shared<Result>(true);
}

ResultPtr RandomForestLayer::OutCore(data::DataPacketPtr& data, data::ParameterPacketPtr& parameter, _type next)
{
	return std::make_shared<Result>(true);
}
