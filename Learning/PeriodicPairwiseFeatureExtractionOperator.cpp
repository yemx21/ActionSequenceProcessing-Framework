#include "PeriodicPairwiseFeatureExtractionOperator.h"
#include "MultivariateTimeSeriesFeatureExtractionLayer.h"
#include "MultivariateTimeSeriesFeaturePacket.h"
#include "Clustering.h"
#include <algorithm>
#include <numeric>
#include <concurrent_unordered_map.h>
#include <boost\range\irange.hpp>
#include <boost\range\algorithm_ext\push_back.hpp>
#include <filesystem>
#include "Persistence1D.h"
#include "..\Graphics\Graphics.h"

using namespace rsdn;
using namespace rsdn::learning;
using namespace rsdn::learning::timeseries;
using namespace rsdn::learning::timeseries::operators;
#include <random>
PeriodicPairwiseFeatureChannelSection::PeriodicPairwiseFeatureChannelSection()
{
	Pairs = std::make_shared<GenericBuffer<std::pair<datatype, datatype>>>();
}

boost::any PeriodicPairwiseFeatureChannelSection::GetDataCore(const std::wstring& key)
{
	if (key.compare(L"pairs") == 0) return Pairs;
	return nullptr;
}

PeriodicPairwiseFeatureSection::PeriodicPairwiseFeatureSection()
{
	Channels = std::make_shared<PeriodicPairwiseFeatureChannelCollection>();
}

boost::any PeriodicPairwiseFeatureSection::GetDataCore(const std::wstring& key)
{
	if (key.compare(L"channels") == 0) return Channels;
	return nullptr;
}

void PeriodicPairwiseFeatureSection::WriteToBSDBWriter(BSDBWriterPtr writer)
{
	if (!writer) return;
	int chnum = (int)Channels->size();
	writer->ReserveChunks(chnum);
	for (int ch = 0; ch < chnum; ch++)
	{
		auto chdata = Channels->at(ch);
		auto chshape = chdata->Pairs->Shape();
		if ((int)chshape.size() == 1)
		{
			int chlen = chshape[0];
			auto chcpu = chdata->Pairs->GetCpu();
			writer->WriteChunk(chcpu, sizeof(std::pair<datatype, datatype>) * chlen);
		}
		else
		{
			writer->WriteChunk(0, 0);
		}
	}
	writer->FinishChunks();
}

static datatype CART_CaclQuality(int lab1, int lab2, const datatype* data, const int* labels, const std::vector<size_t>& valids, int labpernum, int cvfold = 4)
{
	int qualnum = 0;
	double qualsum = 0.0;

	std::map<int, double> olcw;
	std::map<int, double> orcw;

	olcw.insert(std::make_pair(lab1, 0.0));
	olcw.insert(std::make_pair(lab2, 0.0));
	orcw.insert(std::make_pair(lab1, 0.0));
	orcw.insert(std::make_pair(lab2, 0.0));

	std::vector<double> values1;
	std::vector<double> values2;

	for (int i : valids)
	{
		if (labels[i] == lab1)
		{
			orcw[lab1]++;
			values1.push_back(data[i]);
		}
		else if (labels[i] == lab2)
		{
			orcw[lab2]++;
			values2.push_back(data[i]);
		}
	}

	if (values1.empty() || values2.empty()) return 0.0;

	for (int cvi = 0; cvi < cvfold; cvi++)
	{
		int i, best_i = -1;
		double best_val = 0.0;

		std::map<int, double> lcw = olcw;
		std::map<int, double> rcw = orcw;

		std::vector<int> indices1;
		for (int i = 0; i < values1.size(); i++) indices1.push_back(i);
		std::vector<int> indices2;
		for (int i = 0; i < values2.size(); i++) indices2.push_back(i);

		std::vector<datatype> values;
		std::vector<int> labs;

		std::random_shuffle(indices1.begin(), indices1.end(), [](int mx) -> int
		{
			return Random::Generate(true) % mx;
		});
		std::random_shuffle(indices2.begin(), indices2.end(), [](int mx) -> int
		{
			return Random::Generate(true) % mx;
		});

		for (int i = 0; i < labpernum; i++)
		{
			values.push_back(values1[indices1[i]]);
			labs.push_back(lab1);

			values.push_back(values2[indices2[i]]);
			labs.push_back(lab2);
		}
		
		int n = values.size();
		std::vector<int> sorted_idx;
		boost::range::push_back(sorted_idx, boost::irange(0, n));
		std::sort(sorted_idx.begin(), sorted_idx.end(), [&values](int a, int b) -> bool
		{
			return values[a] < values[b];
		});

		double L = 0, R = 0, lsum2 = 0, rsum2 = 0;
		
		{
			double wval = rcw[lab1];
			R += wval;
			rsum2 += wval * wval;
		}

		{
			double wval = rcw[lab2];
			R += wval;
			rsum2 += wval * wval;
		}

		for (i = 0; i < n - 1; i++)
		{
			int curr = sorted_idx[i];
			int next = sorted_idx[i + 1];
			int si = curr;
			L++;
			R--;
			int idx = labs[si];
			double lv = lcw[idx], rv = rcw[idx];
			lsum2 += 2 * lv + 1;
			rsum2 -= 2 * rv - 1;
			lcw[idx] = lv + 1;
			rcw[idx] = rv - 1;

			if (isnan(values[curr]) || isinf(values[next]))
			{
				continue;
			}

			if (values[curr] + DBL_EPSILON < values[next])
			{
				double val = (lsum2 * R + rsum2 * L) / (L * R);

				if (best_val < val)
				{
					best_val = val;
					best_i = i;
				}
			}
		}

		if (best_i != -1)
		{
			qualsum += best_val;
			qualnum++;
		}
	}
	return qualnum == 0 ? 0.0 : qualsum / qualnum;
}

struct PeriodicPairwiseFeatureExtraction_FeatPairHashFun
{
	size_t operator()(const std::pair<int, int>& x) const throw()
	{
		return std::hash<int>()(x.first) ^ std::hash<int>()(x.second);
	}
};

class PeriodicPairwiseFeatureExtraction_FeatPairEqualFun
{
public:
	bool operator() (const std::pair<int, int>& x1, const std::pair<int, int>& x2) const
	{
		return (x1.first == x2.second && x1.second == x2.first) || (x1.first == x2.first && x1.second == x2.second);
	}
};

static void PeriodicPairwiseFeatureExtraction_FindKeyPairs(const datatype* data, int len, concurrency::concurrent_unordered_map<std::pair<int, int>, __int64, PeriodicPairwiseFeatureExtraction_FeatPairHashFun, PeriodicPairwiseFeatureExtraction_FeatPairEqualFun>& pairs, int neighbour)
{

	rsdn::learning::timeseries::details::Persistence1D p1d{};
	if (!p1d.RunPersistence(data, len)) return;

	std::vector<rsdn::learning::timeseries::details::TPairedExtrema> extremas;
	if (!p1d.GetPairedExtrema(extremas)) return;

	int gminidx = p1d.GetGlobalMinimumIndex();
	if (gminidx < 0 || gminidx >= len) return;

	std::vector<datatype> persistences;
	for (const auto& ex : extremas)
	{
		if (!isnan(ex.Persistence))
		{
			if (ex.Persistence < 1e-4) continue;
			persistences.push_back(ex.Persistence);
		}
	}

	datatype kf = 0.0;

	if (persistences.size() > 10)
	{
		if (persistences.empty()) return;

		std::nth_element(persistences.begin(), persistences.begin() + persistences.size() / 2, persistences.end());
		kf = persistences[persistences.size() / 2];

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

	localextremas[gminidx] = data[gminidx];

	std::vector<int> localextremas_indices;
	for (const auto& le : localextremas) localextremas_indices.push_back(le.first);

	int sc = (int)localextremas.size();

	int midindex = (len % 2 == 0) ? len / 2 : (len + 1) / 2;

	if (localextremas.size() > 2)
	{
		/*Optional<datatype> minval;
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
		}*/

		for (int a = 0; a < sc; a++)
		{
			auto sav = localextremas[localextremas_indices[a]];
			auto pab = std::make_pair(localextremas_indices[a]- midindex, -1);
			for (int b = a + 1; b < sc; b++)
			{
				auto sbv = localextremas[localextremas_indices[b]];
				if (abs(localextremas_indices[a] - localextremas_indices[b]) >= neighbour)
				{
					pab.second = localextremas_indices[b] - midindex;
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
			auto pab = std::make_pair(localextremas_indices[0] - midindex, localextremas_indices[1] - midindex);
			auto fpab = pairs.find(pab);
			if (fpab != pairs.end())
				fpab->second++;
			else
				pairs.insert(std::make_pair(pab, 1));
		}
	}

}

ResultPtr PeriodicPairwiseFeatureExtractionOperator::RunCPU(TimeSeriesDataPacketPtr indata, data::ParameterPacketPtr inparam, bool uselabels, std::shared_ptr<GenericBuffer<int>> reflabels, int label_distfun, MultivariateTimeSeriesFeaturePacketPtr outdata, std::atomic<bool>& cancel)
{
	ResultPtr ret = std::make_shared<Result>(false);
	try
	{
		bool hasmidinput = false;
		try
		{
			std::wstring midinputpath = params->GetItem<std::wstring>(L"midinput");
			hasmidinput = true;
		}
		catch (...) {}

		int pairwise_eventneighbour = 2;
		try
		{
			pairwise_eventneighbour = params->GetItem<int>(L"pairwise_eventneighbour");
		}
		catch (...)
		{
			Logger::Get().Report(LogLevel::Info) << "pairwise_eventneighbour is set to defaults 2" << Logger::endl;
		}

		int featurecount = 120;
		try
		{
			featurecount = params->GetItem<int>(L"featurecount");
		}
		catch (...)
		{
			ret->Message = L"invalid parameter: featurecount";
			throw 1;
		}

		if (label_distfun < 1 || label_distfun>1)
		{
			ret->Message = L"invalid parameter: label_distancefunction";
			throw 2;
		}

		std::wstring midinputpath;
		std::wstring outputpath;
		try
		{
			midinputpath = params->GetItem<std::wstring>(L"midinput");
		}
		catch (...) {}

		std::wstring midoutputpath;
		if (featurecount == -1)
		{
			try
			{
				midoutputpath = params->GetItem<std::wstring>(L"midoutput");
			}
			catch (...) {}
		}
		else
		{
			try
			{
				outputpath = params->GetItem<std::wstring>(L"output");
			}
			catch (...)
			{
				ret->Message = L"invalid parameter: output";
				throw 0;
			}
		}

		if (std::experimental::filesystem::exists(midinputpath))
		{
			BSDBReaderPtr reader = std::make_shared<BSDBReader>();
			if (reader->Load(midinputpath, [](const std::wstring& header, int) { return header.compare(L"rsdn::learning::timeseries::operators::periodicpairwisefeatures=>featurepairs") == 0; }))
			{
				if (reader->ChunkCount() == 2)
				{
					long long chnum = 0;
					reader->GetItemValue(L"channelnum", &chnum);
					auto labelsbuf = reader->operator[](L"labels");
					int* reflabs = (int*)labelsbuf->GetCpu();
					int reflabnum = 0;
					reader->GetItemValue(L"labelnum", &reflabnum);

					std::vector<std::vector<std::unordered_map<int, std::unordered_map<int, datatype>>>> qualities;
					std::vector<std::vector<datatype>> Lpairs;
					std::vector<std::vector<datatype>> Rpairs;

					Lpairs.resize(chnum);
					Rpairs.resize(chnum);

					BinaryBufferPtr tmpbuf = std::make_shared<BinaryBuffer>();
					{
						reader->Read(tmpbuf, 0);
						auto tmpcpubuf = tmpbuf->GetCpuReadonlyBuffer();
						for (int ch = 0; ch < chnum; ch++)
						{
							long long cpairnum = 0;
							tmpcpubuf->Read(&cpairnum, 1);
							Lpairs[ch].resize(cpairnum);
							Rpairs[ch].resize(cpairnum);
							tmpcpubuf->Read(Lpairs[ch].data(), cpairnum);
							tmpcpubuf->Read(Rpairs[ch].data(), cpairnum);
						}
					}

					{
						/*qualities*/
						reader->Read(tmpbuf, 1);
						auto tmpcpubuf = tmpbuf->GetCpuReadonlyBuffer();
						datatype* qucpu = (datatype*)tmpbuf->GetCpu();
						qualities.resize(chnum);
						for (int ch = 0; ch < chnum; ch++)
						{
							long long cfeatnum = 0;
							tmpcpubuf->Read(&cfeatnum, 1); 
							qualities[ch].resize(cfeatnum);
							for (int pi = 0; pi < cfeatnum; pi++)
							{
								auto& cqualities = qualities[ch][pi];
								for (int l1 = 0; l1 < reflabnum; l1++)
								{
									for (int l2 = 0; l2 < reflabnum; l2++)
									{
										tmpcpubuf->Read(&cqualities[reflabs[l1]][reflabs[l2]], 1);
									}
								}
							}
						}
					}

					std::vector<double> maxqs;
					maxqs.resize(chnum, 0.0);

					std::vector<std::vector<std::pair<double, std::pair<datatype, datatype>>>> lraqs;
					lraqs.resize(chnum);

					double sumavgqs = 0.0;
					for (int ch = 0; ch < chnum; ch++)
					{
						int cfeatnum = Lpairs[ch].size();
						std::vector<double> sumqs;
						for (int fi = 0; fi < cfeatnum; fi++)
						{
							double sumq = 0.0;

							for (int m = 1; m < 9; m++)
							{
								for (int n = m + 1; n <= 9; n++)
								{
									sumq += qualities[ch][fi][m][n];
								}
							}
							sumqs.push_back(sumq);
							lraqs[ch].push_back(std::make_pair(sumq, std::make_pair(Lpairs[ch][fi], Rpairs[ch][fi])));
						}
						
						auto avg = sumqs.empty() ? 0.0 : std::accumulate(sumqs.begin(), sumqs.end(), 0.0) / sumqs.size();
						sumavgqs += avg;
						maxqs[ch] = avg;
					}

					std::vector<int> selectchfrontcount;
					selectchfrontcount.resize(chnum, 0);

					for (int ch = 0; ch < chnum; ch++)
					{
						int cselectchfrontcount = (int)floor(maxqs[ch] * (double)featurecount / sumavgqs + 0.5);

						double cqs = maxqs[ch];

						if (abs(cqs) > DBL_EPSILON)
						{
							selectchfrontcount[ch] = cselectchfrontcount;
						}
						else
						{
							selectchfrontcount[ch] = 0;
						}
					}

					PeriodicPairwiseFeatureSectionPtr ppfsptr = std::make_shared<PeriodicPairwiseFeatureSection>();
					for (int ch = 0; ch < chnum; ch++)
					{
						PeriodicPairwiseFeatureChannelSectionPtr ppfcsptr = std::make_shared<PeriodicPairwiseFeatureChannelSection>();
						ppfcsptr->Pairs->Reshape(selectchfrontcount[ch]);
						auto paircpu = ppfcsptr->Pairs->GetCpu();

						const auto& lsq = lraqs[ch];

						std::vector<int> sorted_idx;
						boost::range::push_back(sorted_idx, boost::irange(0, (int)lsq.size()));
						std::sort(sorted_idx.begin(), sorted_idx.end(), [&lsq](int a, int b) -> bool
						{
							return lsq[a].first > lsq[b].first;
						});

						for (int i = 0; i < selectchfrontcount[ch] && i < sorted_idx.size(); i++)
						{
							paircpu->first = lsq[sorted_idx[i]].second.first;
							paircpu->second = lsq[sorted_idx[i]].second.second;
							paircpu++;
						}

						ppfsptr->Channels->push_back(ppfcsptr);
					}
					outdata->SetSection(L"periodicpairwisefeatures", ppfsptr);

					if (ppfsptr)
					{
						BSDBWriterPtr writer = std::make_shared<BSDBWriter>();
						if (writer->Open(outputpath, L"rsdn::learning::timeseries::operators::periodicpairwisefeatures=>minedfeaturepairs", 1))
						{
							writer->WriteItem(L"featurecount", &featurecount, sizeof(int));
							writer->WriteItem(L"uselabels", &uselabels, sizeof(bool));
							writer->WriteItem(L"label_distfun", &label_distfun, sizeof(int));
							writer->FinishItems();
							ppfsptr->WriteToBSDBWriter(writer);
							writer->Close();
						}
					}

					ret->State = true;
					return ret;
				}
			}
		}
		else if (label_distfun == 1 && uselabels)
		{
			auto normcolsec = outdata->GetSection(L"templates");
			auto chnum = normcolsec->GetChildCount();

			auto reflabs = reflabels->GetCpu();
			int reflabnum = reflabels->Shape()[0];

			std::vector<std::vector<std::unordered_map<int, std::unordered_map<int, datatype>>>> qualities;
			std::vector<std::vector<datatype>> Lpairs;
			std::vector<std::vector<datatype>> Rpairs;
			qualities.resize(chnum);
			Lpairs.resize(chnum);
			Rpairs.resize(chnum);

			int job = Runtime::CpuJobCount();

			for (int ch = 0; ch < chnum; ch++)
			{
				auto chsec = normcolsec->GetChild(ch);
				auto featuresbuf = chsec->GetData<Buffer>(L"features");
				auto labelsbuf = chsec->GetData<GenericBuffer<int>>(L"labels");

				auto memlabs = labelsbuf->GetCpu();

				int featnum = featuresbuf->Shape()[0];
				int tempnum = featuresbuf->Shape()[1];

				BufferPtr timesbuf = std::make_shared<Buffer>();
				timesbuf->Reshape(featnum);
				auto ttimes = timesbuf->GetCpu();
				for (int i = 0; i < featnum; i++)
				{
					ttimes[i] = (datatype)i / (datatype)featnum;
				}

				std::vector<std::vector<datatype>> samples;
				std::vector<std::vector<int>> classes;
				std::vector<datatype*> features;
				std::vector<int*> labels;
				std::vector<datatype*> times;
				samples.resize(job);

				for (int j = 0; j < job; j++) samples[j].resize(featnum * 3);

				classes.resize(job);
				features.resize(job, featuresbuf->GetCpu());
				times.resize(job, timesbuf->GetCpu());
				labels.resize(job, labelsbuf->GetCpu());

				concurrency::concurrent_unordered_map<std::pair<int, int>, __int64, PeriodicPairwiseFeatureExtraction_FeatPairHashFun, PeriodicPairwiseFeatureExtraction_FeatPairEqualFun> real_feats;

				#pragma omp parallel for num_threads(job)
				for (int n = 0; n < tempnum; n++)
				{
					int thread_id = omp_get_thread_num();
					std::vector<datatype>& feats = samples[thread_id];
					datatype* featscpu = feats.data();
					const auto mem_feats = features[thread_id];
					feats.clear();
					memcpy(featscpu, mem_feats, sizeof(datatype)* featnum);
					memcpy(featscpu + featnum, mem_feats, sizeof(datatype)* featnum);
					memcpy(featscpu + featnum * 2, mem_feats, sizeof(datatype)* featnum);

					for (int fi = featnum/2, fn = 0; fn < featnum; fn++, fi++)
					{
						PeriodicPairwiseFeatureExtraction_FindKeyPairs(featscpu + fi, featnum, real_feats, pairwise_eventneighbour);
					}
				}

				auto& featpairLs = Lpairs[ch];
				auto& featpairRs = Rpairs[ch];

				for (const auto& rf : real_feats)
				{
					featpairLs.push_back((datatype)rf.first.first / (datatype)featnum);
					featpairRs.push_back((datatype)rf.first.second / (datatype)featnum);
				}

				int featpairnum = featpairLs.size();

				std::vector<std::unordered_map<int, std::unordered_map<int, datatype>>>& cqualities = qualities[ch];
				cqualities.resize(featpairnum);

				BufferPtr featsbuf = std::make_shared<Buffer>(tempnum, featnum * 3);
				std::shared_ptr<GenericBuffer<int>> clssbuf = std::make_shared<GenericBuffer<int>>(tempnum, featnum * 3);
				
				size_t allelementcount = tempnum * featnum * 3;

				for (int fi = 0; fi < featpairnum; fi++)
				{
					datatype* feats = featsbuf->GetCpu();
					int* clss = clssbuf->GetCpu();

					#pragma omp parallel for num_threads(job)
					for (int i = 0; i < tempnum; i++)
					{
						datatype* i_feats = feats + i* featnum * 3;
						int* i_clss = clss + i* featnum * 3;

						auto mem_feats = features[0] + featnum * i;
						auto mem_labs = labels[0] + featnum * i;
						auto mem_times = times[0] + featnum * i;

						for (int j = 0; j < featnum; j++)
						{
							datatype* j_feats = i_feats + j * 3;
							int* j_clss = i_clss + j * 3;

							datatype featj = (datatype)j / (featnum - 1);

							datatype featL = featj + featpairLs[fi];
							datatype featR = featj + featpairRs[fi];

							int leftidx = (featL > 1.0 ? (featL - 1.0) : (featL < 0.0 ? (featL + 1.0) : featL))* (featnum - 1);
							int rightidx = (featR > 1.0 ? (featR - 1.0) : (featR < 0.0 ? (featR + 1.0) : featR))* (featnum - 1);

							if (leftidx < 0) leftidx = 0;
							if (leftidx > featnum - 1) leftidx = featnum - 1;

							if (rightidx < 0) rightidx = 0;
							if (rightidx > featnum - 1) rightidx = featnum - 1;

							/*left part*/
							{
								//int leftidx = (featL < 0.5 ? (0.5 + featL) : (featL -0.5)) * (featnum - 1);
								auto val = (mem_feats[leftidx] - mem_feats[j]) / (mem_times[leftidx] - mem_times[j]);
								if (!isnan(val) && !isinf(val))
								{
									j_feats[0] = val;
									j_clss[0] = mem_labs[j];
								}
								else
								{
									j_clss[0] = -1;
								}
							}
							/*right part*/
							{
								//int rightidx = (featR < 0.5 ? (0.5 + featR) : (featR - 0.5)) * (featnum - 1);
								auto val = (mem_feats[j] - mem_feats[rightidx]) / (mem_times[j] - mem_times[rightidx]);
								if (!isnan(val) && !isinf(val))
								{
									j_feats[1] = val;
									j_clss[1] = mem_labs[j];
								}
								else
								{
									j_clss[1] = -1;
								}
							}
							/*all part*/
							{
								auto val = (mem_feats[leftidx] - mem_feats[rightidx]) / (mem_times[leftidx] - mem_times[rightidx]);
								if (!isnan(val) && !isinf(val))
								{
									j_feats[2] = val;
									j_clss[2] = mem_labs[j];
								}
								else
								{
									j_clss[2] = -1;
								}
							}
						}
					}

					std::unordered_map<int, int> clsscounts;
					for (int l1 = 0; l1 < reflabnum; l1++)
					{
						clsscounts[reflabs[l1]] = 0;
					}

					std::vector<size_t> valids;
					for (size_t n = 0; n < allelementcount; n++)
					{
						if (*clss == -1) continue;
						valids.push_back(n);
						clsscounts[*clss]++;
						clss++;
					}
					clss = clssbuf->GetCpu();
					int clsspernum = (*clsscounts.begin()).second;

					for (const auto& clpc : clsscounts)
					{
						if (clpc.second < clsspernum)
						{
							clsspernum = clpc.second;
						}
					}

					std::unordered_map<int, std::unordered_map<int, datatype>>& subquailities = cqualities[fi];
					for (int l1 = 0; l1 < reflabnum; l1++)
					{
						int lab1 = reflabs[l1];
						for (int l2 = 0; l2 < reflabnum; l2++)
						{
							int lab2 = reflabs[l2];
							subquailities[l1][l2] = CART_CaclQuality(lab1, lab2, feats, clss, valids, clsspernum);
						}
					}
				}

				Logger::Get().Report(rsdn::LogLevel::Info) << L"channel " << ch << " feature qualities are computed" << Logger::endl;
				Logger::Get().Report(rsdn::LogLevel::Info) << L"channel " << ch << " has " << featpairnum << " feature pairs" << Logger::endl;

				/*for (int fi = 0; fi < featpairnum; fi++)
				{
					Logger::Get().Report(rsdn::LogLevel::Info) << Lpairs[ch][fi] <<", " << Rpairs[ch][fi] << Logger::endl;
				}*/

			}

			if (featurecount == -1)
			{
				BSDBWriterPtr writer = std::make_shared<BSDBWriter>();
				if (writer->Open(midoutputpath, L"rsdn::learning::timeseries::operators::periodicpairwisefeatures=>featurepairs", 1))
				{
					writer->WriteItem(L"uselabels", &uselabels, sizeof(bool));
					writer->WriteItem(L"label_distfun", &label_distfun, sizeof(int));
					writer->WriteItem(L"labelnum", &reflabnum, sizeof(int));
					writer->WriteItem(L"labels", reflabs, sizeof(int)* reflabnum);
					writer->WriteItem(L"channelnum", &chnum, sizeof(long long));
					writer->FinishItems();

					writer->ReserveChunks(2);

					/*featpairLRs*/
					writer->StartChunk();
					for (int ch = 0; ch < chnum; ch++)
					{
						long long cfeatpairnum = Lpairs[ch].size();
						writer->WriteToChunk(&cfeatpairnum, sizeof(long long));
						writer->WriteToChunk(Lpairs[ch].data(), sizeof(datatype)* cfeatpairnum);
						writer->WriteToChunk(Rpairs[ch].data(), sizeof(datatype)* cfeatpairnum);
					}
					writer->EndChunk();

					/*qualities*/
					writer->StartChunk();
					for (int ch = 0; ch < chnum; ch++)
					{
						long long cfeatnum = Lpairs[ch].size();
						writer->WriteToChunk(&cfeatnum, sizeof(long long));
						for (int pi = 0; pi < cfeatnum; pi++)
						{
							for (int l1 = 0; l1 < reflabnum; l1++)
							{
								for (int l2 = 0; l2 < reflabnum; l2++)
								{
									writer->WriteToChunk(&qualities[ch][pi][reflabs[l1]][reflabs[l2]], sizeof(datatype));
								}
							}
						}
					}
					writer->EndChunk();

					writer->FinishChunks();
					writer->Close();
				}			
				ret->State = true;
				return ret;
			}

			std::vector<double> maxqs;
			maxqs.resize(chnum, 0.0);

			std::vector<std::vector<std::pair<double, std::pair<datatype, datatype>>>> lraqs;
			lraqs.resize(chnum);

			double sumavgqs = 0.0;
			for (int ch = 0; ch < chnum; ch++)
			{
				std::vector<double> sumqs;
				int cpairnum = Lpairs[ch].size();
				for (int fi =0; fi< cpairnum; fi++)
				{
					double sumq = 0.0;

					for (int m = 1; m < 9; m++)
					{
						for (int n = m + 1; n <= 9; n++)
						{
							sumq += qualities[ch][fi][m][n];
						}
					}
					sumqs.push_back(sumq);
					lraqs[ch].push_back(std::make_pair(sumq, std::make_pair(Lpairs[ch][fi], Rpairs[ch][fi])));
				}
				
				auto avg = sumqs.empty() ? 0.0 : std::accumulate(sumqs.begin(), sumqs.end(), 0.0) / sumqs.size();
				sumavgqs += avg;
				maxqs[ch] = avg;
			}

			std::vector<int> selectchfrontcount;
			selectchfrontcount.resize(chnum, 0);

			for (int ch = 0; ch < chnum; ch++)
			{
				int cselectchfrontcount = (int)floor(maxqs[ch] * (double)featurecount / sumavgqs + 0.5);

				double cqs = maxqs[ch];

				if (abs(cqs) > DBL_EPSILON)
				{
					selectchfrontcount[ch] = (abs(cqs) < DBL_EPSILON) ? 0 : (int)floor((double)cselectchfrontcount * cqs / cqs + 0.5);
				}
				else
				{
					selectchfrontcount[ch] = 0;
				}
			}

			PeriodicPairwiseFeatureSectionPtr ppfsptr = std::make_shared<PeriodicPairwiseFeatureSection>();
			for (int ch = 0; ch < chnum; ch++)
			{
				PeriodicPairwiseFeatureChannelSectionPtr ppfcsptr = std::make_shared<PeriodicPairwiseFeatureChannelSection>();
				ppfcsptr->Pairs->Reshape(selectchfrontcount[ch]);
				auto paircpu = ppfcsptr->Pairs->GetCpu();

				const auto& lsq = lraqs[ch];

				std::vector<int> sorted_idx;
				boost::range::push_back(sorted_idx, boost::irange(0, (int)lsq.size()));
				std::sort(sorted_idx.begin(), sorted_idx.end(), [&lsq](int a, int b) -> bool
				{
					return lsq[a].first > lsq[b].first;
				});
				for (int i = 0; i < selectchfrontcount[ch] && i < sorted_idx.size(); i++)
				{
					paircpu->first = lsq[sorted_idx[i]].second.first;
					paircpu->second = lsq[sorted_idx[i]].second.second;
					paircpu++;
				}

				ppfsptr->Channels->push_back(ppfcsptr);
			}
			outdata->SetSection(L"periodicpairwisefeatures", ppfsptr);

			if (ppfsptr)
			{
				BSDBWriterPtr writer = std::make_shared<BSDBWriter>();
				if (writer->Open(outputpath, L"rsdn::learning::timeseries::operators::periodicpairwisefeatures=>minedfeaturepairs", 1))
				{
					writer->WriteItem(L"featurecount", &featurecount, sizeof(int));
					writer->WriteItem(L"uselabels", &uselabels, sizeof(bool));
					writer->WriteItem(L"label_distfun", &label_distfun, sizeof(int));
					writer->FinishItems();
					ppfsptr->WriteToBSDBWriter(writer);
					writer->Close();
				}
			}
			ret->State = true;
		}
	}
	catch (const std::exception& excep)
	{
		ret->Error = Converter::Convert(excep.what());
	}
	catch (...)
	{

	}
	return ret;
}

ResultPtr PeriodicPairwiseFeatureExtractionOperator::ConfigCore(data::ParameterPacketPtr param)
{
	params = param;
	return std::make_shared<Result>(true);
}

ResultPtr PeriodicPairwiseFeatureExtractionOperator::Run(TimeSeriesDataPacketPtr indata, data::ParameterPacketPtr inparam, _type intype, data::ParameterPacketPtr param, MultivariateTimeSeriesFeaturePacketPtr outdata, std::atomic<bool>& cancel)
{
	ResultPtr ret = std::make_shared<Result>(false);

	/*load joint registration*/
	bool uselabels = true;
	std::shared_ptr<GenericBuffer<int>> labels;
	int dist_fun = 1;

	bool hasmidinput = false;
	try
	{
		std::wstring midinputpath = params->GetItem<std::wstring>(L"midinput");
		hasmidinput = true;
	}
	catch (...) {}

	try
	{
		uselabels = params->GetItem<bool>(L"use_labels");
	}
	catch (...)
	{
		if (!hasmidinput)
		{
			ret->Message = L"invalid parameter: use_labels";
			return ret;
		}
	}

	try
	{
		labels = std::dynamic_pointer_cast<GenericBuffer<int>>(params->GetItemEx(L"labels"));
	}
	catch (...)
	{
		if (!hasmidinput)
		{
			ret->Message = L"invalid parameter: labels";
			return ret;
		}
	}

	try
	{
		dist_fun = params->GetItem<int>(L"label_distancefunction");
	}
	catch (...)
	{
		if (!hasmidinput)
		{
			ret->Message = L"invalid parameter: label_distancefunction";
			return ret;
		}
	}

	if (Runtime::Mode() == ComputationMode::CPU)
	{
		ret = RunCPU(indata, inparam, uselabels, labels, dist_fun, outdata, cancel);
	}
	else
	{
		throw std::exception("gpu mode is not implemented");
	}

	return ret;
}

ResultPtr PeriodicPairwiseFeatureExtractionOperator::RunBatchCPU(int batch, TimeSeriesDataPacketPtr indata, data::ParameterPacketPtr inparam, bool uselabels, std::shared_ptr<GenericBuffer<int>> reflabels, int label_distfun, MultivariateTimeSeriesFeaturePacketPtr outdata, std::atomic<bool>& cancel)
{
	ResultPtr ret = std::make_shared<Result>(false);
	try
	{
		int pairwise_resolution = 50;
		try
		{
			pairwise_resolution = params->GetItem<int>(L"pairwise_resolution");
		}
		catch (...)
		{
			ret->Message = L"invalid parameter: pairwise_resolution";
			throw 0;
		}

		if (label_distfun !=2)
		{
			ret->Message = L"invalid parameter: label_distancefunction";
			throw 2;
		}

		std::wstring midoutputpath;
		try
		{
			midoutputpath = params->GetItem<std::wstring>(L"midoutput");
		}
		catch (...)
		{
		}

		std::wstring midoutputdir;
		try
		{
			midoutputdir = params->GetItem<std::wstring>(L"midoutputdir");
			midoutputpath = midoutputdir + L"feat_" + std::to_wstring(batch) + L".rsdn";
		}
		catch (...)
		{
			
		}

		if (label_distfun == 2 && uselabels)
		{
			int pairwise_centroidwise = 10;
			try
			{
				pairwise_centroidwise = params->GetItem<int>(L"pairwise_centroidwise");
			}
			catch (...)
			{
				ret->Message = L"invalid parameter: pairwise_centroidwise";
				throw 0;
			}

			auto normcolsec = outdata->GetSection(L"templates");
			auto chnum = normcolsec->GetChildCount();

			auto reflabs = reflabels->GetCpu();
			int reflabnum = reflabels->Shape()[0];

			std::vector<datatype> featpairLs;
			std::vector<datatype> featpairRs;
			std::vector<long long> featpairLidx;
			std::vector<long long> featpairRidx;
			std::vector<long long> featpairAidx;
			{
				std::vector<datatype> paircands;
				datatype pairstep = 0.5f / pairwise_resolution;
				for (int p = -pairwise_resolution; p <= pairwise_resolution; p++)
				{
					paircands.push_back(pairstep * p);
				}
				do
				{
					size_t curidx = featpairLs.size();
					featpairLs.push_back(paircands[0]);
					featpairRs.push_back(paircands[1]);
					if (paircands[0] < 0.0 && abs(paircands[1]) < pairstep * 0.5)
						featpairLidx.push_back(curidx);
					else if (paircands[1] > 0.0 && abs(paircands[0]) < pairstep * 0.5)
						featpairRidx.push_back(curidx);
					else
						featpairAidx.push_back(curidx);

				} while (rsdn::next_combination(paircands.begin(), paircands.begin() + 2, paircands.end()));
			}
			size_t featpairnum = featpairLs.size();
			std::vector<std::vector<std::unordered_map<int, std::unordered_map<int, datatype>>>> qualities;
			qualities.resize(chnum);

			for (int ch = 0; ch < chnum; ch++)
			{
				auto chsec = normcolsec->GetChild(ch);
				auto featuresbuf = chsec->GetData<Buffer>(L"features");
				auto labelsbuf = chsec->GetData<GenericBuffer<int>>(L"labels");

				auto memlabs = labelsbuf->GetCpu();

				int tempnum = featuresbuf->Shape()[0];
				int featnum = featuresbuf->Shape()[1];
				
				int h2fnum = featnum / 2;
				std::vector<concurrency::concurrent_vector<datatype>> samples;
				std::vector<concurrency::concurrent_vector<int>> classes;
				std::vector<datatype*> features;
				std::vector<int*> labels;
				samples.resize(Runtime::CpuJobCount());
				classes.resize(Runtime::CpuJobCount());
				features.resize(Runtime::CpuJobCount(), featuresbuf->GetCpu());
				labels.resize(Runtime::CpuJobCount(), labelsbuf->GetCpu());

				std::vector<std::unordered_map<int, std::unordered_map<int, datatype>>>& cqualities = qualities[ch];
				cqualities.resize(featpairnum);

				std::atomic<size_t> compnum = 0;
				std::atomic<size_t> comptotal = 0;
				#pragma omp parallel for num_threads(Runtime::CpuJobCount())
				for (int n = 0; n < featpairnum; n++)
				{
					int thread_id = omp_get_thread_num();
					concurrency::concurrent_vector<datatype>& feats = samples[thread_id];
					concurrency::concurrent_vector<int>& clss = classes[thread_id];

					feats.clear();
					clss.clear();

					auto mem_feats = features[thread_id];
					auto mem_labs = labels[thread_id];

					int jstart = featnum / 2 - pairwise_centroidwise / 2;
					int jend = featnum / 2 + pairwise_centroidwise / 2;

					if (jstart < 0) jstart = 0;
					if (jend > featnum - 1) jend = featnum - 1;

					for (int i = 0; i < tempnum; i++)
					{
						for (int j = jstart; j <= jend; j++)
						{
							datatype featj = (datatype)j / (featnum - 1);
							datatype featL = featj + featpairLs[n];
							datatype featR = featj + featpairRs[n];

							int leftidx = (featL > 1.0 ? (featL - 1.0) : (featL < 0.0 ? (featL + 1.0) : featL))* (featnum - 1);
							int rightidx = (featR > 1.0 ? (featR - 1.0) : (featR < 0.0 ? (featR + 1.0) : featR))* (featnum - 1);

							if (leftidx < 0) leftidx = 0;
							if (leftidx > featnum - 1) leftidx = featnum - 1;

							if (rightidx < 0) rightidx = 0;
							if (rightidx > featnum - 1) rightidx = featnum - 1;

							/*left part*/
							{
								//int leftidx = (featL < 0.5 ? (0.5 + featL) : (featL -0.5)) * (featnum - 1);
								auto val = (mem_feats[leftidx] - mem_feats[j]);
								if (!isnan(val) && !isinf(val))
								{
									feats.push_back(val);
									clss.push_back(mem_labs[i]);
								}
							}
							/*right part*/
							{
								//int rightidx = (featR < 0.5 ? (0.5 + featR) : (featR - 0.5)) * (featnum - 1);
								auto val = (mem_feats[j] - mem_feats[rightidx]);
								if (!isnan(val) && !isinf(val))
								{
									feats.push_back(val);
									clss.push_back(mem_labs[i]);
								}
							}
							/*all part*/
							{
								//int leftidx = (featL < 0.5 ? (0.5 + featL) : (featL - 0.5)) * (featnum - 1);
								int rightidx = (featR < 0.5 ? (0.5 + featR) : (featR - 0.5)) * (featnum - 1);
								auto val = (mem_feats[leftidx] - mem_feats[rightidx]);
								if (!isnan(val) && !isinf(val))
								{
									feats.push_back(val);
									clss.push_back(mem_labs[i]);
								}
							}
						}
						mem_feats += featnum;
					}

					std::unordered_map<int, int> clsscounts;
					for (int l1 = 0; l1 < reflabnum; l1++)
					{
						clsscounts[reflabs[l1]] = 0;
					}
					for (int l : clss)
					{
						clsscounts[l]++;
					}
					int clsspernum = (*clsscounts.begin()).second;

					for (const auto& clpc : clsscounts)
					{
						if (clpc.second < clsspernum)
						{
							clsspernum = clpc.second;
						}
					}

					std::unordered_map<int, std::unordered_map<int, datatype>>& subquailities = cqualities[n];
					for (int l1 = 0; l1 < reflabnum; l1++)
					{
						int lab1 = reflabs[l1];
						for (int l2 = l1+1; l2 < reflabnum; l2++)
						{
							int lab2 = reflabs[l2];
							auto cq = 0.0; //CART_CaclQuality(lab1, lab2, feats, clss, clsspernum, 1);
							subquailities[l1][l2] = cq;
							subquailities[l2][l1] = cq;
						}
					}

					features[thread_id] = featuresbuf->GetCpu();
					feats.clear();
					compnum++;
					comptotal++;
					if (compnum > 500)
					{
						compnum = 0;
						Logger::Get().Report(rsdn::LogLevel::Info) << L"channel " << ch << " finish " << comptotal << " features" << Logger::endl;
					}
				}

				Logger::Get().Report(rsdn::LogLevel::Info) << L"channel " << ch << " feature qualities are computed" << Logger::endl;
			}


			BSDBWriterPtr writer = std::make_shared<BSDBWriter>();
			if (writer->Open(midoutputpath, L"rsdn::learning::timeseries::operators::periodicpairwisefeatures=>featurepairs", 1))
			{
				writer->WriteItem(L"pairwise_resolution", &pairwise_resolution, sizeof(int));
				writer->WriteItem(L"uselabels", &uselabels, sizeof(bool));
				writer->WriteItem(L"label_distfun", &label_distfun, sizeof(int));
				writer->WriteItem(L"labelnum", &reflabnum, sizeof(int));
				writer->WriteItem(L"labels", reflabs, sizeof(int)* reflabnum);
				writer->WriteItem(L"channelnum", &chnum, sizeof(long long));
				long long featpairnum_real = featpairnum;
				writer->WriteItem(L"featpairnum", &featpairnum_real, sizeof(long long));
				writer->FinishItems();

				writer->ReserveChunks(6);

				/*featpairLs*/
				writer->WriteChunk(featpairLs.data(), sizeof(datatype)* featpairnum);

				/*featpairRs*/
				writer->WriteChunk(featpairRs.data(), sizeof(datatype)* featpairnum);

				/*featpairLidx*/
				writer->StartChunk();
				long long featpairLidx_num = featpairLidx.size();
				writer->WriteToChunk(&featpairLidx_num, sizeof(long long));
				writer->WriteToChunk(featpairLidx.data(), sizeof(long long)* featpairLidx_num);
				writer->EndChunk();

				/*featpairRidx*/
				writer->StartChunk();
				long long featpairRidx_num = featpairRidx.size();
				writer->WriteToChunk(&featpairRidx_num, sizeof(long long));
				writer->WriteToChunk(featpairRidx.data(), sizeof(long long)* featpairRidx_num);
				writer->EndChunk();

				/*featpairAidx*/
				writer->StartChunk();
				long long featpairAidx_num = featpairAidx.size();
				writer->WriteToChunk(&featpairAidx_num, sizeof(long long));
				writer->WriteToChunk(featpairAidx.data(), sizeof(long long)* featpairAidx_num);
				writer->EndChunk();

				/*qualities*/
				writer->StartChunk();
				for (int ch = 0; ch < chnum; ch++)
				{
					for (int pi = 0; pi < featpairnum; pi++)
					{
						for (int l1 = 0; l1 < reflabnum; l1++)
						{
							for (int l2 = 0; l2 < reflabnum; l2++)
							{
								writer->WriteToChunk(&qualities[ch][pi][reflabs[l1]][reflabs[l2]], sizeof(datatype));
							}
						}
					}
				}
				writer->EndChunk();

				writer->FinishChunks();
				writer->Close();
			}
			ret->State = true;
			return ret;
		}
	}
	catch (const std::exception& excep)
	{
		ret->Error = Converter::Convert(excep.what());
	}
	catch (...)
	{

	}
	return ret;
}


ResultPtr PeriodicPairwiseFeatureExtractionOperator::RunBatch(int batch, TimeSeriesDataPacketPtr indata, data::ParameterPacketPtr inparam, _type intype, data::ParameterPacketPtr param, MultivariateTimeSeriesFeaturePacketPtr outdata, std::atomic<bool>& cancel)
{
	ResultPtr ret = std::make_shared<Result>(false);

	/*load joint registration*/
	bool uselabels = true;
	std::shared_ptr<GenericBuffer<int>> labels;
	int dist_fun = 2;

	try
	{
		uselabels = params->GetItem<bool>(L"use_labels");
	}
	catch (...)
	{
	}

	try
	{
		labels = std::dynamic_pointer_cast<GenericBuffer<int>>(params->GetItemEx(L"labels"));
	}
	catch (...)
	{
	}

	try
	{
		dist_fun = params->GetItem<int>(L"label_distancefunction");
	}
	catch (...)
	{
	}

	if (Runtime::Mode() == ComputationMode::CPU)
	{
		ret = RunBatchCPU(batch, indata, inparam, uselabels, labels, dist_fun, outdata, cancel);
	}
	else
	{
		throw std::exception("gpu mode is not implemented");
	}

	return ret;
}