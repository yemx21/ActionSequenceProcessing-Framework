#include "ActionPairwiseLayer.h"
#include "STRUCTDataLayer.h"
#include "ActionDataPacket.h"
#include "ActionRepresentationDataPacket.h"
#include "Spline.h"
#include "..\Graphics\Graphics.h"
using namespace rsdn::learning::timeseries;

ActionPairwiseLayer::ActionPairwiseLayer()
{

}

ActionPairwiseLayer::~ActionPairwiseLayer()
{

}

std::unique_ptr<std::future<ResultPtr>> ActionPairwiseLayer::RunAsync(std::atomic<bool>& cancel)
{
	return std::make_unique<std::future<ResultPtr>>(std::async(std::launch::deferred, [&cancel, &param = params]()
	{
		ResultPtr ret = std::make_shared<Result>(false);
		try
		{
			std::wstring input;
			try
			{
				input = param->GetItem<std::wstring>(L"input");
			}
			catch (...)
			{
				ret->Message = L"invalid parameter: input";
				return ret;
			}

			std::wstring output;
			try
			{
				output = param->GetItem<std::wstring>(L"output");
			}
			catch (...)
			{
				ret->Message = L"invalid parameter: output";
				return ret;
			}

			int neigbourcompress = 3;
			try
			{
				neigbourcompress = param->GetItem<int>(L"neigbourcompress");
			}
			catch (...)
			{
				Logger::Get().Report(LogLevel::Info) << "neigbourcompress sets to default 3" << Logger::endl;
			}

			int job = Runtime::CpuJobCount();

			FileStream infile;
			if (infile.Open(input.c_str(), FileAccess::Read, FileShare::Read, FileMode::Open))
			{
				int real_featsnum = 0;
				infile.Read(real_featsnum, false);

				int tmpfirst = -1;
				int tmpsecond = -1;
				std::vector<std::pair<int, int>> pairs;
				std::set<int> ufirst;
				std::map<int, double> uniqueelement;
				std::map<int, double> uniqueelementcounter;

				std::vector<__int64> pcounters;
				pairs.resize(real_featsnum);
				pcounters.resize(real_featsnum, 0);
				int icounter = 0;
				for (int i = 0; i < real_featsnum; i++)
				{
					infile.Read(tmpfirst, false);
					infile.Read(tmpsecond, false);
					infile.Read(pcounters[icounter], false);

					if (tmpfirst < tmpsecond)
					{
						pairs[icounter].first = tmpfirst;
						pairs[icounter].second = tmpsecond;
						ufirst.insert(tmpfirst);
					}
					else
					{
						pairs[icounter].first = tmpsecond;
						pairs[icounter].second = tmpfirst;
						ufirst.insert(tmpsecond);
					}

					if (uniqueelement.find(pairs[icounter].first) != uniqueelement.end())
					{
						uniqueelement[pairs[icounter].first] += pcounters[icounter];
						uniqueelementcounter[pairs[icounter].first]++;
					}
					else
					{
						uniqueelement[pairs[icounter].first] = pcounters[icounter];
						uniqueelementcounter[pairs[icounter].first] = 1;
					}

					if (uniqueelement.find(pairs[icounter].second) != uniqueelement.end())
					{
						uniqueelement[pairs[icounter].second] += pcounters[icounter];
						uniqueelementcounter[pairs[icounter].second] ++;
					}
					else
					{
						uniqueelement[pairs[icounter].second] = pcounters[icounter];
						uniqueelementcounter[pairs[icounter].second] = 1;
					}
					icounter++;
				}
				infile.Close();

				if (icounter < real_featsnum)
				{
					pairs.erase(pairs.begin() + icounter, pairs.end());
					pcounters.erase(pcounters.begin() + icounter, pcounters.end());
				}

				double sumelement = 0;
				for (auto& ue : uniqueelement)
				{
					ue.second /= uniqueelementcounter[ue.first];
					sumelement += ue.second;
				}

				int groupmin = (int)floor((double)pairs.size() / uniqueelement.size() + 0.5);

				for (int uf : ufirst)
				{
					std::map<int, double> scores;
					int paircount = pairs.size();
					for (int i = 0; i < paircount; i++)
					{
						const auto& p = pairs[i];
						if (abs(p.first - uf) <= neigbourcompress || abs(p.second - uf) <= neigbourcompress)
						{
							scores[i] = pcounters[i];
						}
					}
					if (scores.empty()) continue;
					std::vector<int> us;
					for (const auto& sc : scores) us.push_back(sc.first);
					std::sort(us.begin(), us.end(), [&scores](const int a, const int b)->bool
					{
						return scores[a] > scores[b];
					});

					std::vector<int> removes;
					for (int i = groupmin; i< us.size(); i++) removes.push_back(us[i]);
					std::sort(removes.begin(), removes.end());
					while (!removes.empty())
					{
						int cr = removes[0];
						pairs.erase(pairs.begin() + cr);
						pcounters.erase(pcounters.begin() + cr);
						removes.erase(removes.begin());
						for (auto& r : removes) r--;
					}
				}

				std::set<int> usecond;
				for (const auto& p : pairs) usecond.insert(p.second);
				
				for (int uf : usecond)
				{
					std::map<int, double> scores;
					int paircount = pairs.size();
					for (int i = 0; i < paircount; i++)
					{
						const auto& p = pairs[i];
						if (abs(p.first - uf) <= neigbourcompress || abs(p.second - uf) <= neigbourcompress)
						{
							scores[i] = pcounters[i];
						}
					}
					if (scores.empty()) continue;
					std::vector<int> us;
					for (const auto& sc : scores) us.push_back(sc.first);
					std::sort(us.begin(), us.end(), [&scores](const int a, const int b)->bool
					{
						return scores[a] > scores[b];
					});

					std::vector<int> removes;
					for (int i = groupmin; i < us.size(); i++) removes.push_back(us[i]);
					std::sort(removes.begin(), removes.end());
					while (!removes.empty())
					{
						int cr = removes[0];
						pairs.erase(pairs.begin() + cr);
						pcounters.erase(pcounters.begin() + cr);
						removes.erase(removes.begin());
						for (auto& r : removes) r--;
					}
				}

				FileStream outfile;
				if (outfile.Open(output.c_str(), FileAccess::Write, FileShare::Write, FileMode::Create))
				{
					int sellen = (int)pairs.size();

					outfile.Write((char*)&sellen, sizeof(int), 0, sizeof(int));

					for (const auto& rf : pairs)
					{
						outfile.Write((char*)&rf.first, sizeof(int), 0, sizeof(int));
						outfile.Write((char*)&rf.second, sizeof(int), 0, sizeof(int));
					}
					outfile.Flush();
					outfile.Close();


					/*rsdn::graphics::Plot2D plot{};
					int lc = 0;
					for (const auto& sp : pairs)
					{
						plot.AddScatter(sp.first, lc , 3, 255, 120, 120);
						plot.AddScatter(sp.second, lc, 3, 120, 255, 120);
						lc++;
					}
					plot.SaveAsPdf(L"F:\\STRUCT\\train\\sel.pdf", 1200, 800);*/

					ret->State = true;
				}
			}
		}
		catch (const std::exception& excep)
		{
			ret->Error = Converter::Convert(excep.what());
		}
		return ret;
	}));
}


ResultPtr ActionPairwiseLayer::IsSupportConnectFrom(_type prev)
{
	return std::make_shared<Result>(true);
}

ResultPtr ActionPairwiseLayer::IsSupportConnectTo(_type next)
{
	return std::make_shared<Result>(false, L"next layer is unsupported");
}

ResultPtr ActionPairwiseLayer::ReadyCore()
{
	return std::make_shared<Result>(true);
}

ResultPtr ActionPairwiseLayer::InCore(data::DataPacketPtr data, data::ParameterPacketPtr prevparameter, _type prev)
{
	return std::make_shared<Result>(true);
}

ResultPtr ActionPairwiseLayer::OutCore(data::DataPacketPtr& data, data::ParameterPacketPtr& parameter, _type next)
{
	parameter = params;
	return std::make_shared<Result>(true);
}

ResultPtr ActionPairwiseLayer::ConfigCore(data::ParameterPacketPtr param)
{
	params = param;
	return std::make_shared<Result>(true);
}
