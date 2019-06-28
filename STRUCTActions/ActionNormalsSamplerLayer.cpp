#include "ActionNormalsSamplerLayer.h"
#include "STRUCTDataLayer.h"
#include "ActionDataPacket.h"
#include "ActionRepresentationDataPacket.h"
#include "Spline.h"
using namespace rsdn::learning::timeseries;
ActionNormalsSamplerLayer::ActionNormalsSamplerLayer()
{

}

ActionNormalsSamplerLayer::~ActionNormalsSamplerLayer()
{

}

class ActionNormalsSamplerLayer_PairCandicate
{
public:
	int A;
	int B;
	int Valid;
	std::vector<datatype> Factors;
	std::vector<sizetype> Indices;
	datatype Sparsity;
	datatype Quality;
	datatype Split;

	ActionNormalsSamplerLayer_PairCandicate() : Sparsity(0.0)
	{

	}
};

std::unique_ptr<std::future<ResultPtr>> ActionNormalsSamplerLayer::RunAsync(std::atomic<bool>& cancel)
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

			std::wstring pairsinput;
			try
			{
				pairsinput = param->GetItem<std::wstring>(L"pairsinput");
			}
			catch (...)
			{
				ret->Message = L"invalid parameter: pairsinput";
				return ret;
			}

			std::wstring samplesoutput;
			try
			{
				samplesoutput = param->GetItem<std::wstring>(L"samplesoutput");
			}
			catch (...)
			{
				ret->Message = L"invalid parameter: samplesoutput";
				return ret;
			}

			std::shared_ptr<GenericBuffer<int>> labels;
			try
			{
				labels = std::dynamic_pointer_cast<GenericBuffer<int>>(param->GetItemEx(L"labels"));
			}
			catch (...)
			{
				ret->Message = L"invalid parameter: labels";
				return ret;
			}

			bool interp = true;
			try
			{
				interp = param->GetItem<bool>(L"interp");
			}
			catch (...)
			{
				Logger::Get().Report(LogLevel::Info) << "interp sets to default \"true\"" << Logger::endl;
			}

			std::vector<std::pair<int, int>> real_feats;
			rsdn::data::BSDBReaderPtr pairfile = std::make_shared<rsdn::data::BSDBReader>();
			if (pairfile->Load(pairsinput, [](const std::wstring& head, int ver)-> bool
			{
				return head.compare(L"rsdn::learning::timeseries::operators:PairwiseFieldOperator=>featurecandicates") == 0 && ver > -1;
			}))
			{
				std::vector<ActionNormalsSamplerLayer_PairCandicate> candicates;
				{
					BinaryBufferPtr buf = std::make_shared<BinaryBuffer>();
					if (pairfile->Read(buf, 0))
					{
						auto cpubuf = buf->GetCpuReadonlyBuffer();
						__int64 candicatenum = 0ll;
						cpubuf->Read(&candicatenum, 1);
						for (__int64 i = 0; i < candicatenum; i++)
						{
							int vinum = 0;
							cpubuf->Read(&vinum, 1);
							ActionNormalsSamplerLayer_PairCandicate pc{};
							pc.Factors.resize(vinum);
							pc.Indices.resize(vinum);
							cpubuf->Read(&pc.A, 1);
							cpubuf->Read(&pc.B, 1);
							cpubuf->Read(pc.Indices.data(), vinum);
							cpubuf->Read(pc.Factors.data(), vinum);
							cpubuf->Read(&pc.Sparsity, 1);
							cpubuf->Read(&pc.Quality, 1);
							cpubuf->Read(&pc.Split, 1);
							pc.Valid = pc.Indices.size(); 
							candicates.push_back(pc);
						}
					}
				}

				int candnum = candicates.size();

				if(candnum>0)
				{
					int job = Runtime::CpuJobCount();

					SampleChunk samples{};
					auto labelscpu = labels->GetCpu();
					int labellen = labels->Shape()[0];
					for (int i = 0; i < labellen; i++)
					{
						samples.AddUniqueLabel(labelscpu[i]);					
					}
					samples.SetFeatureCount(candnum);
					
					FileStream infile;
					if (infile.Open(samplesinput.c_str(), FileAccess::Read, FileShare::Read, FileMode::Open))
					{
						int desiredframes = 0;
						infile.Read<int>(desiredframes, false);
						int chcount = 0;
						infile.Read<int>(chcount, false);
						int seqrecorded = 0;
						infile.Read<int>(seqrecorded, false);

						BufferPtr rawseqchunkbuf = std::make_shared<Buffer>(chcount, desiredframes);
						datatype* rawseqchunk = rawseqchunkbuf->GetCpu();

						if (!interp)
						{
							std::vector<datatype> sampledata;
							sampledata.resize(candnum);
							for (int i = 0; i < seqrecorded; i++)
							{
								int seqlab = -1;
								infile.Read<int>(seqlab, false);
								for (int di = 0; di < desiredframes; di++)
								{
									auto tmpcpu = rawseqchunk + di*chcount;
									infile.Read((char*)tmpcpu, sizeof(datatype)* chcount, 0, sizeof(datatype)* chcount);
								}
								for (int fi = 0; fi < candnum; fi++)
								{
									const auto& ca = candicates[fi];
									auto tmpcpu1 = rawseqchunk + ca.A * chcount;
									auto tmpcpu2 = rawseqchunk + ca.B * chcount;

									datatype val = 0;
									for (int fn = 0; fn < ca.Valid; fn++)
									{
										val += ca.Factors[fn] * (tmpcpu1[ca.Indices[fn]] - tmpcpu2[ca.Indices[fn]]);
									}
									sampledata[fi] = (isinf(val) || isnan(val)) ? 0 : val;
								}
								samples.PushWithLabel(sampledata.data(), seqlab);
								Logger::Get().Report(LogLevel::Info) << i + 1 << L"sequences finished" << Logger::endl;
							}
						}
						else
						{
							BufferPtr seqchunkbuf = std::make_shared<Buffer>(chcount, desiredframes);
							datatype* seqchunk = seqchunkbuf->GetCpu();

							std::vector<datatype> iframes;
							iframes.resize(desiredframes);
							for (int j = 0; j < desiredframes; j++) iframes[j] = j;
							std::vector<datatype> sampledata;
							sampledata.resize(candnum);
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

								//if (i < 820)
								/*{
									samples.PushWithLabel(sampledata.data(), seqlab);
									continue;
								}*/

								{
									#pragma omp parallel for num_threads(job)
									for (int j = 0; j < chcount; j++)
									{
										rsdn::learning::timeseries::operators::details::Spline spline;
										if (spline.set_points1(iframes.data(), (seqchunk + j*desiredframes), (size_t)desiredframes))
											spline.interp_points(iframes.data(), (seqchunk + j*desiredframes), (size_t)desiredframes);
									}
								}

								for (int fi = 0; fi < candnum; fi++)
								{
									const auto& ca = candicates[fi];
									auto tmpcpu1 = seqchunk + ca.A * chcount;
									auto tmpcpu2 = seqchunk + ca.B * chcount;
									
									datatype val = 0;
									for (int fn = 0; fn < ca.Valid; fn++)
									{
										val += ca.Factors[fn] * (tmpcpu1[ca.Indices[fn]] - tmpcpu2[ca.Indices[fn]]);
									}
									//val= (datatype)(1.0 / (1.0 + exp(-val)));
									sampledata[fi] = (isinf(val) || isnan(val)) ? 0 : val;
								}

								samples.PushWithLabel(sampledata.data(), seqlab);

								Logger::Get().Report(LogLevel::Info) << i + 1 << L" / " << seqrecorded << L" sequences finished" << Logger::endl;
							}
						}

						samples.Save(samplesoutput);

						infile.Close();
					}
				}
				ret->State = true;
			}
		}
		catch (const std::exception& excep)
		{
			ret->Error = Converter::Convert(excep.what());
		}
		return ret;
	}));
}


ResultPtr ActionNormalsSamplerLayer::IsSupportConnectFrom(_type prev)
{
	return std::make_shared<Result>(true);
}

ResultPtr ActionNormalsSamplerLayer::IsSupportConnectTo(_type next)
{
	return std::make_shared<Result>(false, L"next layer is unsupported");
}

ResultPtr ActionNormalsSamplerLayer::ReadyCore()
{
	return std::make_shared<Result>(true);
}

ResultPtr ActionNormalsSamplerLayer::InCore(data::DataPacketPtr data, data::ParameterPacketPtr prevparameter, _type prev)
{
	return std::make_shared<Result>(true);
}

ResultPtr ActionNormalsSamplerLayer::OutCore(data::DataPacketPtr& data, data::ParameterPacketPtr& parameter, _type next)
{
	parameter = params;
	return std::make_shared<Result>(true);
}

ResultPtr ActionNormalsSamplerLayer::ConfigCore(data::ParameterPacketPtr param)
{
	params = param;
	return std::make_shared<Result>(true);
}
