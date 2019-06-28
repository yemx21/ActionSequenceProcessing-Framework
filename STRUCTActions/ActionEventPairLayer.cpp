#include "ActionEventPairLayer.h"
#include "STRUCTDataLayer.h"
#include "ActionDataPacket.h"
#include "ActionRepresentationDataPacket.h"
#include "Spline.h"
#include <concurrent_unordered_map.h>
#include "Persistence1D.h"
using namespace rsdn::learning::timeseries;

ActionEventPairLayer::ActionEventPairLayer()
{

}

ActionEventPairLayer::~ActionEventPairLayer()
{

}

struct ActionEventPairLayer_FeatPairHashFun
{
	size_t operator()(const std::pair<int, int>& x) const throw()
	{
		return std::hash<int>()(x.first) ^ std::hash<int>()(x.second);
	}
};

class ActionEventPairLayer_FeatPairEqualFun
{
public:
	bool operator() (const std::pair<int, int>& x1, const std::pair<int, int>& x2) const
	{
		return (x1.first == x2.second && x1.second == x2.first) || (x1.first == x2.first && x1.second == x2.second);
	}
};

static void ActionEventPairLayer_FindKeyPairs(const datatype* data, int len, concurrency::concurrent_unordered_map<std::pair<int, int>, __int64, ActionEventPairLayer_FeatPairHashFun, ActionEventPairLayer_FeatPairEqualFun>& pairs, int neighbour)
{
	rsdn::learning::timeseries::details::Persistence1D p1d{};
	if (!p1d.RunPersistence(data, len)) return;

	std::vector<rsdn::learning::timeseries::details::TPairedExtrema> extremas;
	if (!p1d.GetPairedExtrema(extremas)) return;

	std::vector<datatype> persistences;
	for (const auto& ex : extremas)
	{
		if (!isnan(ex.Persistence))
		{
			if (ex.Persistence < 1e-4) continue;
			persistences.push_back(ex.Persistence);
		}
	}

	if (persistences.empty()) return;

	std::nth_element(persistences.begin(), persistences.begin() + persistences.size() / 2, persistences.end());
	datatype kf = persistences[persistences.size() / 2];

	for (auto iter = extremas.begin(); iter != extremas.end();)
	{
		if (iter->MinIndex == 0 || iter->MaxIndex == 0 || iter->MinIndex == len - 1 || iter->MaxIndex == len - 1)
		{
			iter = extremas.erase(iter);
		}
		else
		{
			iter++;
		}
	}

	std::map<int, datatype> localextremas;
	for (const auto& ex : extremas)
	{
		if (!isnan(ex.Persistence))
		{
			if (ex.Persistence > kf)
			{
				if (ex.MinIndex >= 0 && ex.MinIndex < len)
				{
					if (!isnan(data[ex.MinIndex]))
					{
						localextremas[ex.MinIndex] = data[ex.MinIndex];
					}
				}
				if (ex.MaxIndex >= 0 && ex.MaxIndex < len)
				{
					if (!isnan(data[ex.MaxIndex]))
					{
						localextremas[ex.MaxIndex] = data[ex.MaxIndex];
					}
				}
			}
		}
	}
	
	std::vector<int> localextremas_indices;
	for (const auto& le : localextremas) localextremas_indices.push_back(le.first);

	int sc = (int)localextremas.size();

	if (localextremas.size() > 2)
	{
		Optional<datatype> minval;
		Optional<datatype> maxval;

		for (const auto& ex : localextremas)
		{
			if (minval)
			{
				if (ex.second < *minval)
				{
					minval = ex.second;
				}
			}
			else
				minval = ex.second;

			if (maxval)
			{
				if (ex.second > *maxval)
				{
					maxval = ex.second;
				}
			}
			else
				maxval = ex.second;
		}

		for (int a = 0; a < sc; a++)
		{
			auto sav = localextremas[localextremas_indices[a]];
			auto pab = std::make_pair(localextremas_indices[a], -1);
			for (int b = a + 1; b < sc; b++)
			{
				auto sbv = localextremas[localextremas_indices[b]];
				if (abs(localextremas_indices[a] - localextremas_indices[b]) >= neighbour)
				{
					pab.second = localextremas_indices[b];
					auto fpab = pairs.find(pab);
					if (fpab != pairs.end())
						fpab->second++;
					else
						pairs.insert(std::make_pair(pab, 1));
				}
			}
		}
	}
	else if (localextremas.size() == 2)
	{
		if (abs(localextremas_indices[0] - localextremas_indices[1]) >= neighbour)
		{
			auto sav = localextremas[localextremas_indices[0]];
			auto sbv = localextremas[localextremas_indices[1]];
			auto pab = std::make_pair(localextremas_indices[0], localextremas_indices[1]);
			auto fpab = pairs.find(pab);
			if (fpab != pairs.end())
				fpab->second++;
			else
				pairs.insert(std::make_pair(pab, 1));
		}
	}
}

std::unique_ptr<std::future<ResultPtr>> ActionEventPairLayer::RunAsync(std::atomic<bool>& cancel)
{
	return std::make_unique<std::future<ResultPtr>>(std::async(std::launch::deferred, [&cancel, &param = params]()
	{
		ResultPtr ret = std::make_shared<Result>(false);
		try
		{
			std::wstring samplesinput;
			try
			{
				samplesinput = param->GetItem<std::wstring>(L"samplesinput");
			}
			catch (...)
			{
				ret->Message = L"invalid parameter: samplesinput";
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

			int pairwise_eventneighbour = 2;
			try
			{
				pairwise_eventneighbour = param->GetItem<int>(L"pairwise_eventneighbour");
			}
			catch (...)
			{
				Logger::Get().Report(LogLevel::Info) << "pairwise_eventneighbour is set to defaults 2" << Logger::endl;
			}

			int job = Runtime::CpuJobCount();

			FileStream infile;
			if (infile.Open(samplesinput.c_str(), FileAccess::Read, FileShare::Read, FileMode::Open))
			{
				int desiredframes = 0;
				infile.Read<int>(desiredframes, false);
				int chcount = 0;
				infile.Read<int>(chcount, false);
				int seqrecorded = 0;
				infile.Read<int>(seqrecorded, false);

				concurrency::concurrent_unordered_map<std::pair<int, int>, __int64, ActionEventPairLayer_FeatPairHashFun, ActionEventPairLayer_FeatPairEqualFun> real_feats;

				BufferPtr rawseqchunkbuf = std::make_shared<Buffer>(chcount, desiredframes);
				datatype* rawseqchunk = rawseqchunkbuf->GetCpu();
				
				BufferPtr seqchunkbuf = std::make_shared<Buffer>(chcount, desiredframes);
				datatype* seqchunk = seqchunkbuf->GetCpu();

				std::vector<__int64> hist_frames;
				hist_frames.resize(desiredframes, 0);

				std::vector<datatype> iframes;
				iframes.resize(desiredframes);
				for (int j = 0; j < desiredframes; j++) iframes[j] = j;
				for (int i = 0; i < seqrecorded; i++)
				{
					int seqlab = -1;
					infile.Read<int>(seqlab, false);
					for (int di = 0; di < desiredframes; di++)
					{
						auto tmpcpu = rawseqchunk + di*chcount;
						infile.Read((char*)tmpcpu, sizeof(datatype)* chcount, 0, sizeof(datatype)* chcount);
						for (int j = 0; j < chcount; j++)
						{
							(seqchunk + j*desiredframes)[di] = tmpcpu[j];
						}
					}

					{
						#pragma omp parallel for num_threads(job)
						for (int j = 0; j < chcount; j++)
						{
							rsdn::learning::timeseries::operators::details::Spline spline;
							if (spline.set_points1(iframes.data(), (seqchunk + j*desiredframes), (size_t)desiredframes))
							{
								spline.interp_points(iframes.data(), (seqchunk + j*desiredframes), (size_t)desiredframes);
								ActionEventPairLayer_FindKeyPairs((seqchunk + j*desiredframes), desiredframes, real_feats, pairwise_eventneighbour);
							}
						}
					}

					Logger::Get().Report(LogLevel::Info) << i + 1 << L" / " << seqrecorded << L" sequences finished" << Logger::endl;
				}
				infile.Close();

				FileStream outfile;
				if (outfile.Open(output.c_str(), FileAccess::Write, FileShare::Write, FileMode::Create))
				{
					int real_featsnum = (int)real_feats.size();
					outfile.Write((char*)&real_featsnum, sizeof(int), 0, sizeof(int));

					for (const auto& rf : real_feats)
					{
						outfile.Write((char*)&rf.first.first, sizeof(int), 0, sizeof(int));
						outfile.Write((char*)&rf.first.second, sizeof(int), 0, sizeof(int));
						outfile.Write((char*)&rf.second, sizeof(__int64), 0, sizeof(__int64));
					}
					outfile.Flush();
					outfile.Close();
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


ResultPtr ActionEventPairLayer::IsSupportConnectFrom(_type prev)
{
	return std::make_shared<Result>(true);
}

ResultPtr ActionEventPairLayer::IsSupportConnectTo(_type next)
{
	return std::make_shared<Result>(false, L"next layer is unsupported");
}

ResultPtr ActionEventPairLayer::ReadyCore()
{
	return std::make_shared<Result>(true);
}

ResultPtr ActionEventPairLayer::InCore(data::DataPacketPtr data, data::ParameterPacketPtr prevparameter, _type prev)
{
	return std::make_shared<Result>(true);
}

ResultPtr ActionEventPairLayer::OutCore(data::DataPacketPtr& data, data::ParameterPacketPtr& parameter, _type next)
{
	parameter = params;
	return std::make_shared<Result>(true);
}

ResultPtr ActionEventPairLayer::ConfigCore(data::ParameterPacketPtr param)
{
	params = param;
	return std::make_shared<Result>(true);
}
