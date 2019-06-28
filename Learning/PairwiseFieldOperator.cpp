#include "PairwiseFieldOperator.h"
#include "MultivariateTimeSeriesFeatureExtractionLayer.h"
#include "MultivariateTimeSeriesFeaturePacket.h"
#include "DecisionUnitL1.h"
#include <concurrent_unordered_map.h>
#include <unordered_map>
#include <unordered_set>
#include "..\Graphics\Graphics.h"
#include <boost\filesystem.hpp>

using namespace rsdn;
using namespace rsdn::learning;
using namespace rsdn::learning::timeseries;
using namespace rsdn::learning::timeseries::operators;
using namespace rsdn::learning::machines::supervised;


ResultPtr PairwiseFieldOperator::ConfigCore(data::ParameterPacketPtr param)
{
	params = param;
	return std::make_shared<Result>(true);
}

static datatype PairwiseField_CART_FindFactors(const SampleChunk* data, const std::vector<sizetype>& dataindices, int featnum, const std::vector<sizetype>& indexs, const sizetype selcount, std::vector<sizetype>& sels, std::vector<datatype>& factors, datatype& c, datatype& qual, int batchstablity, datatype lr, int maxiter, datatype mindr, datatype rr)
{
	sels.clear();
	sels.resize(selcount);
	for (int i = 0; i < selcount; i++) sels[i] = indexs[i];

	DecisionUnitL1::Train(data,featnum, dataindices, sels, factors, c, qual, batchstablity, lr, maxiter, mindr, rr);
	
	unsigned int sparsity = 0;
	for (int i = 0; i < selcount; i++)
	{
		if (abs(factors[i]) > 1e-6) sparsity++;
	}
	return (datatype)sparsity / (datatype)selcount;
}

class PairwiseField_Candicate
{
public:
	std::vector<datatype> Factors;
	std::vector<sizetype> SelectedIndices;
	datatype Sparsity;
	datatype Quality;
	datatype Split;

	PairwiseField_Candicate(): Sparsity(0.0)
	{

	}

};

class PairwiseField_CandicateCollection
{
public:
	concurrency::concurrent_vector<PairwiseField_Candicate> Items;

	PairwiseField_CandicateCollection()
	{

	}
};

class PairwiseField_Candicate1
{
public:
	int Uid;
	int Id;
	std::vector<datatype> Factors;
	std::vector<sizetype> SelectedIndices;
	std::vector<sizetype> ValidIndices;
	std::vector<datatype> ValidFactors;
	datatype Sparsity;
	datatype Quality;
	datatype Split;
	int Valid;

	PairwiseField_Candicate1() : Sparsity(0.0), Valid(0)
	{

	}

	PairwiseField_Candicate1(int id) : Sparsity(0.0), Id(id), Valid(0)
	{

	}
};

class PairwiseField_Pair
{
public:
	int A;
	int B;
	PairwiseField_Pair() :A(-1), B(-1) {}
	PairwiseField_Pair(int a, int b) : A(a), B(b) {}
};

template<>
struct std::hash<std::pair<int, int>>
{
	size_t operator()(const std::pair<int, int>& x) const throw()
	{
		return std::hash<int>()(x.first) ^ std::hash<int>()(x.second);
	}
};

class EqualFn
{
public:
	bool operator() (const std::pair<int, int>& x1, const std::pair<int, int>& x2) const
	{
		return (x1.first == x2.second && x1.second == x2.first) || (x1.first == x2.first && x1.second == x2.second);
	}
};

static void PairwiseField_CART_SearchOptimalFactors(const SampleChunk* data, PairwiseField_CandicateCollection& cand, int branch, int bs, datatype lr, int maxiter, datatype mindr, datatype rr)
{
	int featnum = data->FeatureCount / 2;
	int selfeatnum = sqrt(featnum);
	if (selfeatnum < 2) selfeatnum = 2;
	if (featnum < 2) selfeatnum = 1;

	std::vector<sizetype> dataindices;
	dataindices.resize(data->Count);
	for (sizetype i = 0; i < data->Count; i++) dataindices[i] = i;
	
	sizetype selcount = sqrt(featnum);
	if (selcount < 2) selcount = 2;
	if (featnum == 1) selcount = 1;
	datatype selsplimit = 0.5 / selcount;

	auto labels = data->UniqueLabels;
	auto labcount = labels.size();
	int job = Runtime::Get().GetCpuJobCount();

	std::vector<std::vector<sizetype>> indices;
	indices.resize(job);
	for (int i = 0; i < job; i++)
	{		
		indices[i].resize(featnum);
		for (int j = 0; j < featnum; j++) indices[i][j] = j;
	}

	#pragma omp parallel for num_threads(job)
	for (int bi = 0; bi < branch; bi++)
	{
		auto& cindices = indices[omp_get_thread_num()];
		std::vector<datatype> fa;
		std::random_shuffle(cindices.begin(), cindices.end(), [](int mx) -> int
		{
			return Random::Generate(true) % mx;
		});
		std::vector<sizetype> ss;
		datatype c = 0.5;
		datatype q = 0.0;
		auto sp = PairwiseField_CART_FindFactors(data, dataindices, featnum, cindices, selcount, ss, fa, c, q, bs, lr, maxiter, mindr, rr);
		if (abs(sp) > selsplimit && q > 0.0)
		{
			PairwiseField_Candicate pc;
			pc.Factors = fa;
			pc.SelectedIndices = ss;
			pc.Sparsity = sp;
			pc.Split = c;
			pc.Quality = q;
			cand.Items.push_back(pc);
		}
	}
}

struct PairwiseFieldOperator_FeatPairCompare
{
	bool operator() (const std::pair<int, int>& lhs, const std::pair<int, int>& rhs) const
	{
		return (lhs.first == rhs.first && lhs.second == rhs.second) || lhs.first == rhs.second && lhs.second == rhs.first;
	}
};

__forceinline bool PairwiseFieldOperator_CombinationEquals(const std::pair<int, int>& a, const std::pair<int, int>& b)
{
	return (a.first == b.first && a.second == b.second) || (a.first == b.second && a.second == b.first);
}

__forceinline bool PairwiseFieldOperator_SelectedIndicesSimilars(const PairwiseField_Candicate1* a, const PairwiseField_Candicate1* b, float simliarity= 0.25)
{
	size_t anum = a->ValidIndices.size();
	size_t bnum = b->ValidIndices.size();

	int same = 0;

	for (int i = 0; i < anum; i++)
	{
		int vi = a->ValidIndices[i];
		for (int j = 0; j < bnum; j++)
		{
			int vj = b->ValidIndices[j];
			if (vi == vj)
			{
				same++;
				break;
			}
		}
	}

	int num = anum > bnum ? anum : bnum;

	float sm = (float)same / (float)num;
	return sm > simliarity;
}

class Filter_PairCandicate
{
public:
	int A;
	int B;
	std::vector<datatype> Factors;
	std::vector<sizetype> Indices;
	datatype Sparsity;
	datatype Quality;
	datatype Split;

	Filter_PairCandicate() : Sparsity(0.0)
	{

	}
};

ResultPtr PairwiseFieldOperator::RunCPU(TimeSeriesDataPacketPtr indata, data::ParameterPacketPtr inparam, MultivariateTimeSeriesFeaturePacketPtr outdata, std::atomic<bool>& cancel)
{
	ResultPtr ret = std::make_shared<Result>(false);
	try
	{
		std::wstring mode;
		try
		{
			mode = params->GetItem<std::wstring>(L"mode");
			if (mode.compare(L"load") != 0 && mode.compare(L"filter") != 0)
			{
				ret->Message = L"invalid parameter: mode must be 'load' or 'filter'";
				return ret;
			}
		}
		catch (...)
		{
			ret->Message = L"invalid parameter: mode";
			return ret;
		}

		if (mode.compare(L"load") == 0)
		{
			std::wstring inputdir;
			try
			{
				inputdir = params->GetItem<std::wstring>(L"inputdir");
			}
			catch (...)
			{
				ret->Message = L"invalid parameter: inputdir";
				return ret;
			}

			std::wstring pairsinput;
			try
			{
				pairsinput = params->GetItem<std::wstring>(L"pairsinput");
			}
			catch (...)
			{
				ret->Message = L"invalid parameter: pairsinput";
				return ret;
			}

			std::wstring pairsoutput;
			try
			{
				pairsoutput = params->GetItem<std::wstring>(L"pairsoutput");
			}
			catch (...)
			{
				ret->Message = L"invalid parameter: pairsoutput";
				return ret;
			}

			float fieldimportancelimit = 0.5;
			try
			{
				fieldimportancelimit = params->GetItem<float>(L"fieldimportancelimit");
			}
			catch (...)
			{

			}

			std::vector<std::pair<int, int>> real_feats;
			FileStream pairfile;
			if (pairfile.Open(pairsinput.c_str(), FileAccess::Read, FileShare::Read, FileMode::Open))
			{
				int pairnum = 0;
				pairfile.Read(pairnum, false);

				for (int si = 0; si < pairnum; si++)
				{
					std::pair<int, int> rf = { -1, -1 };
					pairfile.Read(rf.first, false);
					pairfile.Read(rf.second, false);
					real_feats.push_back(rf);
				}
				pairfile.Close();
			}
			int featcount = (int)real_feats.size();

			std::unordered_map<int, std::set<int>> feats_friends;
			for (int i = 0; i < featcount; i++)
			{
				for (int j = i + 1; j < featcount; j++)
				{
					if (real_feats[i].first == real_feats[j].first || real_feats[i].first == real_feats[j].second || real_feats[i].second == real_feats[j].first || real_feats[i].second == real_feats[j].second)
					{
						feats_friends[i].insert(j);
						feats_friends[j].insert(i);
					}
				}
			}

			for (auto fiter = feats_friends.begin(); fiter != feats_friends.end();)
			{
				if (fiter->second.empty())
					fiter = feats_friends.erase(fiter);
				else
					fiter++;
			}

			std::vector<PairwiseField_Candicate1*> candicates;
			std::unordered_map<int, std::list<PairwiseField_Candicate1*>> cls_candicates;
			for (auto& p : boost::filesystem::directory_iterator(inputdir))
			{
				if (p.path().has_stem())
				{
					FileStream infile;
					if (infile.Open(p.path().c_str(), FileAccess::Read, FileShare::Read, FileMode::Open))
					{
						int cid = -1;
						infile.Read(cid, false);
						if (cid >= 0 && cid < featcount)
						{
							int pccitemcount = 0;
							infile.Read(pccitemcount);

							std::pair<int, int> comb;

							for (int i = 0; i < pccitemcount; i++)
							{
								int factorcount = 0;
								infile.Read(factorcount, false);

								if (factorcount > 0 && comb.first != -1 && comb.second != -1)
								{
									PairwiseField_Candicate1* pc = new PairwiseField_Candicate1(cid);
									pc->Factors.resize(factorcount, 0.0);
									pc->SelectedIndices.resize(factorcount);
									infile.Read((char*)pc->Factors.data(), sizeof(datatype)* factorcount, 0, sizeof(datatype)* factorcount, false);
									infile.Read((char*)pc->SelectedIndices.data(), sizeof(sizetype)* factorcount, 0, sizeof(sizetype)* factorcount, false);
									infile.Read(pc->Sparsity, false);
									infile.Read(pc->Split, false);
									infile.Read(pc->Quality, false);

									std::vector<int> orders;
									orders.resize(factorcount);
									for (int ord = 0; ord < factorcount; ord++) orders[ord] = ord;
									std::vector<datatype> orderedfactors;
									std::vector<sizetype> orderedselectedindices = pc->SelectedIndices;
									std::sort(orders.begin(), orders.end(), [&orderedselectedindices](int a, int b)-> bool
									{
										return orderedselectedindices[a] < orderedselectedindices[b];
									});
									orderedselectedindices.clear();
									for (int ord : orders)
									{
										orderedfactors.push_back(pc->Factors[ord]);
										orderedselectedindices.push_back(pc->SelectedIndices[ord]);
									}

									pc->Factors = orderedfactors;
									pc->SelectedIndices = orderedselectedindices;

									for (int sfi = 0; sfi < pc->Factors.size(); sfi++)
									{
										if (abs(pc->Factors[sfi] > 1e-6))
										{
											pc->ValidIndices.push_back(pc->SelectedIndices[sfi]);
											pc->ValidFactors.push_back(pc->Factors[sfi]);
										}
									}
									pc->Valid = pc->ValidIndices.size();

									pc->Uid = candicates.size();
									cls_candicates[pc->Id].push_back(pc);
									candicates.push_back(pc);
								}
							}
						}
						infile.Close();
					}
				}
			}

			int cls_candicates_num = cls_candicates.size();
			int cls_candicates_ct = 0;
			for (auto& cls : cls_candicates)
			{
				auto& clslist = cls.second;
				std::set<int> clsremoves;

				std::vector<PairwiseField_Candicate1*> clsvec;
				for (PairwiseField_Candicate1* pc : clslist) clsvec.push_back(pc);
				int clsvecnum = (int)clsvec.size();

				for (int c1 = 0; c1 < clsvecnum; c1++)
				{
					PairwiseField_Candicate1* pc1 = clsvec[c1];
					for (auto c2 = c1 + 1; c2 < clsvecnum; c2++)
					{
						PairwiseField_Candicate1* pc2 = clsvec[c2];
						if (PairwiseFieldOperator_SelectedIndicesSimilars(pc1, pc2, 0.25))
						{
							if (pc1->Quality > pc2->Quality)
							{
								clsremoves.insert(pc2->Uid);
							}
							else
							{
								clsremoves.insert(pc1->Uid);
							}
						}
						else
						{
							//clsremoves.insert()
						}
					}
				}

				for (int uid : clsremoves)
				{
					clslist.remove(candicates[uid]);
				}

				//Logger::Get().Report(LogLevel::Info) << "reduction step " << cls_candicates_ct++ <<"/" << cls_candicates_num<< Logger::endl;
			}

			for (const auto& ff : feats_friends)
			{
				std::set<int> c1removes;
				auto& c1 = cls_candicates[ff.first];
				for (PairwiseField_Candicate1* pc1 : c1)
				{
					for (const auto& f : ff.second)
					{
						auto& c2 = cls_candicates[f];

						std::vector<int> uidremoves;
						bool ispc1remove = false;
						for (PairwiseField_Candicate1* pc2 : c2)
						{
							if (PairwiseFieldOperator_SelectedIndicesSimilars(pc1, pc2))
							{
								if (pc1->Quality > pc2->Quality)
								{
									uidremoves.push_back(pc2->Uid);
								}
								else
								{
									ispc1remove = true;
									break;
								}
							}
							else
							{
								if (pc1->Sparsity < pc2->Sparsity)
								{
									uidremoves.push_back(pc2->Uid);
								}
								else
								{
									ispc1remove = true;
									break;
								}
							}
						}

						if (ispc1remove)
						{
							c1removes.insert(pc1->Uid);
						}
						else
						{
							if (!uidremoves.empty())
							{
								for (int uid : uidremoves)
								{
									c2.remove(candicates[uid]);
								}
							}
						}
					}
				}

				for (int uid : c1removes)
				{
					c1.remove(candicates[uid]);
				}
			}

			datatype maxqual = 0.0;
			std::set<int> mms;
			for (const auto& clsc : cls_candicates)
			{
				const auto& clslist = clsc.second;
				for (auto pc : clslist)
				{

					if (pc->Quality > maxqual)
					{
						maxqual = pc->Quality;
					}
				}
			}

			datatype limitqual = maxqual* fieldimportancelimit;
			for (auto& clsc : cls_candicates)
			{
				for (auto pciter = clsc.second.begin(); pciter != clsc.second.end();)
				{
					if ((*pciter)->Quality < limitqual)
					{
						pciter = clsc.second.erase(pciter);
					}
					else
						pciter++;
				}
			}

			std::unordered_map<std::pair<int, int>, std::vector<datatype>> qualities;
			__int64 candicatenum = 0ll;
			for (const auto& clsc : cls_candicates)
			{
				const auto& clslist = clsc.second;
				for (auto pc : clslist)
				{
					if (!pc->ValidIndices.empty())
					{
						std::pair<int, int> comb = { real_feats[pc->Id].first, -1 };
						for (auto cc : pc->ValidIndices)
						{
							comb.second = cc;
							qualities[comb].push_back(pc->Quality);
						}

						comb = { real_feats[pc->Id].second, -1 };
						for (auto cc : pc->ValidIndices)
						{
							comb.second = cc;
							qualities[comb].push_back(pc->Quality);
						}
						candicatenum++;
					}
				}
			}

			{
				rsdn::graphics::Image img{ 100, 1670 };
				for (const auto& qual : qualities)
				{
					img.Draw(qual.first.first, qual.first.second, 80, 255, 140);
				}
				img.SaveAsPng(L"H:\\s1.png");
			}

			try
			{
				rsdn::data::BSDBWriterPtr writer = std::make_shared<rsdn::data::BSDBWriter>();
				if (writer->Open(pairsoutput, L"rsdn::learning::timeseries::operators:PairwiseFieldOperator=>featurecandicates", 0))
				{
					writer->FinishItems();
					writer->ReserveChunks(1);
					writer->StartChunk();
					writer->WriteToChunk(&candicatenum, sizeof(__int64));
					for (const auto& clsc : cls_candicates)
					{
						const auto& clslist = clsc.second;
						for (auto pc : clslist)
						{
							if (!pc->ValidIndices.empty())
							{
								writer->WriteToChunk(&pc->Valid, sizeof(int));
								writer->WriteToChunk(&real_feats[pc->Id].first, sizeof(int));
								writer->WriteToChunk(&real_feats[pc->Id].second, sizeof(int));
								writer->WriteToChunk(pc->ValidIndices.data(), sizeof(sizetype)* pc->Valid);
								writer->WriteToChunk(pc->ValidFactors.data(), sizeof(datatype)* pc->Valid);
								writer->WriteToChunk(&pc->Sparsity, sizeof(datatype));
								writer->WriteToChunk(&pc->Quality, sizeof(datatype));
								writer->WriteToChunk(&pc->Split, sizeof(datatype));
							}
						}
					}
					writer->EndChunk();
					writer->FinishChunks();
					writer->Close();
				}
				else
					ret->Message = L"can not save file";
			}
			catch (...)
			{
				ret->Message = L"error occurred when try to write data to file";
			}

			for (auto c : candicates)
			{
				delete c;
				c = nullptr;
			}

			ret->State = true;
		}
		else if (mode.compare(L"filter") == 0)
		{
			std::wstring pairsinput;
			try
			{
				pairsinput = params->GetItem<std::wstring>(L"pairsinput");
			}
			catch (...)
			{
				ret->Message = L"invalid parameter: pairsinput";
				return ret;
			}

			std::wstring pairsoutput;
			try
			{
				pairsoutput = params->GetItem<std::wstring>(L"pairsoutput");
			}
			catch (...)
			{
				ret->Message = L"invalid parameter: pairsoutput";
				return ret;
			}

			int toprank = 1000;
			try
			{
				toprank = params->GetItem<int>(L"toprank");
			}
			catch (...)
			{
				Logger::Get().Report(LogLevel::Info) << "toprank is set to default 1000" << Logger::endl;
			}

			rsdn::data::BSDBReaderPtr reader = std::make_shared<rsdn::data::BSDBReader>();
			if (reader->Load(pairsinput, [](const std::wstring& head, int ver)-> bool
			{
				return head.compare(L"rsdn::learning::timeseries::operators:PairwiseFieldOperator=>featurecandicates") == 0 && ver > -1;
			}))
			{
				BinaryBufferPtr buf = std::make_shared<BinaryBuffer>();
				if (reader->Read(buf, 0))
				{
					std::vector<Filter_PairCandicate> selcandicates;
					std::unordered_map<std::pair<int, int>, std::vector<Filter_PairCandicate>, std::hash<std::pair<int, int>>, EqualFn> candicates;
					std::unordered_map<std::pair<int, int>, datatype> candavgqual;
					double cansumavgqual = 0;
					std::unordered_map<std::pair<int, int>, int> candselnums;
					auto cpubuf = buf->GetCpuReadonlyBuffer();
					__int64 candicatenum = 0ll;
					cpubuf->Read(&candicatenum, 1);
					for (__int64 i = 0; i < candicatenum; i++)
					{
						int vinum = 0;
						cpubuf->Read(&vinum, 1);
						Filter_PairCandicate pc{};
						pc.Factors.resize(vinum);
						pc.Indices.resize(vinum);
						cpubuf->Read(&pc.A, 1);
						cpubuf->Read(&pc.B, 1);
						cpubuf->Read(pc.Indices.data(), vinum);
						cpubuf->Read(pc.Factors.data(), vinum);
						cpubuf->Read(&pc.Sparsity, 1);
						cpubuf->Read(&pc.Quality, 1);
						cpubuf->Read(&pc.Split, 1);
						candicates[std::make_pair(pc.A, pc.B)].push_back(pc);
					}

					for (const auto& cand : candicates)
					{
						datatype avgq = 0;
						for (const auto& c : cand.second)
						{
							avgq += c.Quality;
						}
						candavgqual[cand.first] = avgq / (datatype)cand.second.size();
						cansumavgqual += candavgqual[cand.first];
					}

					for (const auto& candq : candavgqual)
					{
						candselnums[candq.first] = (int)floor((double)candq.second / cansumavgqual* (double)toprank + 0.5);
					}

					for (auto& cand : candicates)
					{
						auto& candlist = cand.second;
						int cn = candlist.size();
						std::vector<__int64> indices;
						indices.resize(cn);
						for (__int64 i = 0; i < cn; i++) indices[i] = i;
						std::sort(indices.begin(), indices.end(), [&candlist](__int64 a, __int64 b)-> bool
						{
							return candlist[a].Sparsity < candlist[b].Sparsity;
						});

						int ln = candselnums[cand.first];
						for (int nn = 0; nn < ln && nn< cn; nn++)
						{
							selcandicates.push_back(candlist[nn]);
						}
					}

					__int64 topnum = selcandicates.size();

					rsdn::data::BSDBWriterPtr writer = std::make_shared<rsdn::data::BSDBWriter>();
					if (writer->Open(pairsoutput, L"rsdn::learning::timeseries::operators:PairwiseFieldOperator=>featurecandicates", 0))
					{
						writer->FinishItems();
						writer->ReserveChunks(1);
						writer->StartChunk();
						writer->WriteToChunk(&topnum, sizeof(__int64));
						for (int n = 0; n<topnum; n++)
						{
							const auto& pc = selcandicates[n];
							int vinum = (int)pc.Indices.size();
							writer->WriteToChunk(&vinum, sizeof(int));
							writer->WriteToChunk(&pc.A, sizeof(int));
							writer->WriteToChunk(&pc.B, sizeof(int));
							writer->WriteToChunk(pc.Indices.data(), sizeof(sizetype)* vinum);
							writer->WriteToChunk(pc.Factors.data(), sizeof(datatype)* vinum);
							writer->WriteToChunk(&pc.Sparsity, sizeof(datatype));
							writer->WriteToChunk(&pc.Quality, sizeof(datatype));
							writer->WriteToChunk(&pc.Split, sizeof(datatype));
						}
						writer->EndChunk();
						writer->FinishChunks();
						writer->Close();

						ret->State = true;
					}
				}

				
			}
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

ResultPtr PairwiseFieldOperator::Run(TimeSeriesDataPacketPtr indata, data::ParameterPacketPtr inparam, _type intype, data::ParameterPacketPtr param, MultivariateTimeSeriesFeaturePacketPtr outdata, std::atomic<bool>& cancel)
{
	ResultPtr ret = std::make_shared<Result>(false);

	if (Runtime::Mode() == ComputationMode::CPU)
	{
		ret = RunCPU(indata, inparam, outdata, cancel);
	}
	else
	{
		throw std::exception("gpu mode is not implemented");
	}

	return ret;
}

ResultPtr PairwiseFieldOperator::RunBatch(int batch, TimeSeriesDataPacketPtr indata, data::ParameterPacketPtr inparam, _type intype, data::ParameterPacketPtr param, MultivariateTimeSeriesFeaturePacketPtr outdata, std::atomic<bool>& cancel)
{
	ResultPtr ret = std::make_shared<Result>(false);

	if (Runtime::Mode() == ComputationMode::CPU)
	{
		ret = RunBatchCPU(batch, indata, inparam, outdata, cancel);
	}
	else
	{
		throw std::exception("gpu mode is not implemented");
	}

	return ret;
}

ResultPtr PairwiseFieldOperator::RunBatchCPU(int batch, TimeSeriesDataPacketPtr indata, data::ParameterPacketPtr inparam, MultivariateTimeSeriesFeaturePacketPtr outdata, std::atomic<bool>& cancel)
{
	ResultPtr ret = std::make_shared<Result>(false);
	try
	{
		std::wstring mode;
		try
		{
			mode = params->GetItem<std::wstring>(L"mode");
			if (mode.compare(L"save") != 0)
			{
				ret->Message = L"invalid parameter: mode must be 'save'";
				return ret;
			}
		}
		catch (...)
		{
			ret->Message = L"invalid parameter: mode";
			return ret;
		}

		auto colsec = indata->GetSection(L"pairwise_collection");
		auto data = colsec->GetData1<SampleChunk>(L"samples");

		if (data->Count == 0) { ret->Message = L"empty data";  return ret;}

		int batchstability = 50;
		try
		{
			batchstability = params->GetItem<int>(L"batchstability");
		}
		catch (...)
		{
		}

		float learningrate = 0.02;
		try
		{
			learningrate = params->GetItem<float>(L"learningrate");
		}
		catch (...)
		{
		}

		int maxiter = 2000;
		try
		{
			maxiter = params->GetItem<int>(L"maxiter");
		}
		catch (...)
		{
		}

		float convergence = 0.01;
		try
		{
			convergence = params->GetItem<float>(L"convergence");
		}
		catch (...)
		{
		}

		float regularization = 0.0001;
		try
		{
			regularization = params->GetItem<float>(L"regularization");
		}
		catch (...)
		{
		}

		int branch = 10;
		try
		{
			branch = params->GetItem<int>(L"branch");
		}
		catch (...)
		{

		}

		std::shared_ptr<GenericBuffer<int>> labels;
		try
		{
			labels = std::dynamic_pointer_cast<GenericBuffer<int>>(params->GetItemEx(L"labels"));
			int labelsnum = labels->Shape()[0];
			auto labelscpu = labels->GetCpu();
			data->ClearUniqueLabel();
			for (int i = 0; i < labelsnum; i++)
			{
				data->AddUniqueLabel(labelscpu[i]);
			}
		}
		catch (...)
		{
			ret->Message = L"invalid parameter: labels";
			return ret;
		}

		std::wstring outputdir;
		std::wstring outputpath;
		std::shared_ptr<int> colid = 0;
		try
		{
			outputdir = params->GetItem<std::wstring>(L"outputdir");

			colid = colsec->GetData<int>(L"id");

			outputpath = outputdir + L"field_" + std::to_wstring(*colid) + L".db";
		}
		catch (...)
		{
			ret->Message = L"invalid parameter: outputdir";
			return ret;
		}

		PairwiseField_CandicateCollection pcc;
		PairwiseField_CART_SearchOptimalFactors(data, pcc, branch, batchstability, learningrate, maxiter, convergence, regularization);

		FileStream outfile;
		if (outfile.Open(outputpath.c_str(), FileAccess::Write, FileShare::Write, FileMode::Create))
		{
			int oid = *colid;
			outfile.Write((char*)&oid, sizeof(int), 0, sizeof(int));
			int pccitemcount = pcc.Items.size();
			outfile.Write((char*)&pccitemcount, sizeof(int), 0, sizeof(int));

			for (const auto& pc : pcc.Items)
			{
				int factorcount = pc.Factors.size();
				outfile.Write((char*)&factorcount, sizeof(int), 0, sizeof(int));
				outfile.Write((char*)pc.Factors.data(), sizeof(datatype)* factorcount, 0, sizeof(datatype)* factorcount);
				outfile.Write((char*)pc.SelectedIndices.data(), sizeof(sizetype)* factorcount, 0, sizeof(sizetype)* factorcount);
				outfile.Write((char*)&pc.Sparsity, sizeof(datatype), 0, sizeof(datatype));
				outfile.Write((char*)&pc.Split, sizeof(datatype), 0, sizeof(datatype));
				outfile.Write((char*)&pc.Quality, sizeof(datatype), 0, sizeof(datatype));
				outfile.Flush();
			}
			outfile.Close();
		}

		Logger::Get().Report(rsdn::LogLevel::Info) << L"batch[" << batch << L"] is finished" << Logger::endl;
		Logger::Get().Report(rsdn::LogLevel::Info) << L"save to " << outputpath << Logger::endl;
		outdata->SetTemporary(L"batch", std::make_shared<int>(batch));
		
		ret->State = true;
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
