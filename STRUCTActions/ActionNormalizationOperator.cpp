#include "ActionNormalizationOperator.h"
#include "Interpolator.h"
#include <numeric>
#include "So3LieAlgebra.h"
#include <boost\filesystem.hpp>
#include <boost/lambda/bind.hpp>

using namespace rsdn;
using namespace rsdn::learning;
using namespace rsdn::learning::timeseries;
using namespace rsdn::learning::timeseries::operators;
using namespace rsdn::learning::timeseries::details;

#define ACTION_JOINTCOUNT 25
#define ACTION_JOINTROOT 1
#define ACTION_JOINTNORMALNODE 24
constexpr int ACTION_JOINTNORMAL[ACTION_JOINTNORMALNODE] = { 0, 2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24 };

ActionNormalizationOperator::ActionNormalizationOperator():isexclusive(false), batchcount(0)
{

}

ResultPtr ActionNormalizationOperator::ConfigCore(data::ParameterPacketPtr param)
{
	params = param;

	try
	{
		isexclusive = params->GetItem<bool>(L"batch");
	}
	catch(...)
	{
		batchcount = 0u;
		batches.clear();
		featsamples.clear();
		isexclusive = false;
	}

	return std::make_shared<Result>(true);
}

bool ActionNormalizationOperator::IsExclusive() const
{
	return isexclusive;
}

unsigned int ActionNormalizationOperator::GetBatchCount()
{
	return batchcount;
}

ResultPtr ActionNormalizationOperator::PrepareBatchAsync(ActionDataPacketPtr indata, data::ParameterPacketPtr inparam, _type intype, data::ParameterPacketPtr param, ActionRepresentationDataPacketPtr outdata, std::atomic<bool>& cancel)
{
	ResultPtr ret = std::make_shared<Result>(false);
	batchcount = 0u;
	batches.clear();
	featsamples.clear();

	std::wstring submode = L"save";
	try
	{
		submode = params->GetItem<std::wstring>(L"submode");
	}
	catch (...)
	{
		Logger::Get().Report(LogLevel::Info) << "submode changed to default \"save\"" << Logger::endl;
	}

	try
	{
		if (submode.compare(L"load") == 0)
		{
			std::wstring samplesinputdir;
			try
			{
				samplesinputdir = params->GetItem<std::wstring>(L"samplesinputdir");
			}
			catch (...)
			{
				ret->Message = L"invalid parameter: samplesinputdir"; 
				return ret;
			}

			int batchstart = 0;
			try
			{
				batchstart = params->GetItem<int>(L"batchstart");
			}
			catch (...)
			{
			}

			int batchend = -1;
			try
			{
				batchend = params->GetItem<int>(L"batchend");
				if (batchend!=-1 &&  batchend < batchstart)
				{
					ret->Message = L"invalid parameter: batchend should be equal or larger than batchstart";
					return ret;
				}
			}
			catch (...)
			{
				batchend = -1;
			}

			batches.clear();
			featsamples.clear();
			std::map<int, std::wstring> pids;
			
			for (auto& p : boost::filesystem::directory_iterator(samplesinputdir))
			{
				if (p.path().has_stem())
				{
					if (p.path().has_extension())
					{
						if (p.path().extension().compare(L".featpair")==0)
						{
							FileStream infile;
							if (infile.Open(p.path().c_str(), FileAccess::Read, FileShare::Read, FileMode::Open))
							{
								int pid = -1;
								infile.Read<int>(pid, false);
								infile.Close();
								pids[pid] = p.path().wstring();
							}
						}
						else if (p.path().extension().compare(L".samples") == 0)
						{
							FileStream infile;
							if (infile.Open(p.path().c_str(), FileAccess::Read, FileShare::Read, FileMode::Open))
							{
								int pid = -1;
								infile.Read<int>(pid, false);
								infile.Close();
								featsamples[pid] = p.path().wstring();
							}
						}
					}
				}
			}

			for (const auto& p : pids)
			{
				batches.push_back(p.second);
			}

			if (batchstart > 0) batches.erase(batches.begin(), batches.begin() + batchstart);
			if (batchend != -1)
			{
				batchend -= batchstart;
				if (batchend > batches.size() - 1) batchend = batches.size() - 1;
				if (batchend < batches.size() - 1) batches.erase(batches.begin() + batchend +1, batches.end());
			}
			batchcount = (int)batches.size();
			ret->State = batchcount > 0;
		}
	}
	catch (...)
	{

	}
	return ret;
}

ResultPtr ActionNormalizationOperator::RunBatchAsync(ActionDataPacketPtr indata, data::ParameterPacketPtr inparam, _type intype, data::ParameterPacketPtr param, ActionRepresentationDataPacketPtr outdata, std::atomic<bool>& cancel, unsigned int batch)
{
	ResultPtr ret = std::make_shared<Result>(false);

	if (batch < batchcount)
	{
		std::wstring submode = L"load";
		try
		{
			submode = params->GetItem<std::wstring>(L"submode");
		}
		catch (...)
		{
			Logger::Get().Report(LogLevel::Info) << "submode changed to default \"save\"" << Logger::endl;
		}

		try
		{
			if (submode.compare(L"load") == 0)
			{
				std::wstring samplesinput = batches[batch];
				int leftsi = -1;
				int rightsi = -1;
				FileStream infile;
				int featid = 0;
				if (infile.Open(samplesinput.c_str(), FileAccess::Read, FileShare::Read, FileMode::Open))
				{
					
					infile.Read<int>(featid, false);
					infile.Read<int>(leftsi, false);
					infile.Read<int>(rightsi, false);
					infile.Close();
				}

				FileStream lfile;
				FileStream rfile;

				if (featsamples.find(leftsi) != featsamples.end() && featsamples.find(rightsi) != featsamples.end())
				{
					if (lfile.Open(featsamples[leftsi].c_str(), FileAccess::Read, FileShare::Read, FileMode::Open))
					{
						if (rfile.Open(featsamples[rightsi].c_str(), FileAccess::Read, FileShare::Read, FileMode::Open))
						{
							int tmpfeatid = 0;
							lfile.Read<int>(tmpfeatid, false);
							rfile.Read<int>(tmpfeatid, false);

							int l_chcount = 0;							
							int r_chcount = 0;
							int l_seqrecorded = 0;
							int r_seqrecorded = 0;

							lfile.Read<int>(l_chcount, false);
							lfile.Read<int>(l_seqrecorded, false);
							rfile.Read<int>(r_chcount, false);
							rfile.Read<int>(r_seqrecorded, false);

							if (l_chcount == r_chcount&& l_seqrecorded == r_seqrecorded)
							{
								int ch2 = l_chcount * 2;
								BufferPtr tmpbuf = std::make_shared<Buffer>(ch2);
								auto tmpbufcpu = tmpbuf->GetCpu();
								outdata->PairwiseSection->Samples->SetFeatureCount(ch2);
								outdata->PairwiseSection->Samples->Resize(0, true, true);
								outdata->PairwiseSection->SetData(L"id", std::make_shared<int>(featid));

								std::map<int, int> clss;

								for (int i = 0; i < l_seqrecorded; i++)
								{
									int seqlab = -1;
									lfile.Read<int>(seqlab, false);
									lfile.Read((char*)tmpbufcpu, sizeof(datatype)* l_chcount, 0, sizeof(datatype)* l_chcount);
									rfile.Read<int>(seqlab, false);
									rfile.Read((char*)(tmpbufcpu + l_chcount), sizeof(datatype)* r_chcount, 0, sizeof(datatype)* r_chcount);

									bool skip = false;
									for (int j = 0; j < ch2; j++)
									{
										if (isnan(tmpbufcpu[j]) || isinf(tmpbufcpu[j]))
										{
											skip = true;
											break;
										}
									}
									if (skip) continue;
									outdata->PairwiseSection->Samples->PushWithLabel(tmpbufcpu, seqlab);
									clss[seqlab]++;
								}

								lfile.Close();
								outdata->SetSection(L"pairwise_collection", outdata->PairwiseSection);
								ret->State = true;
							}					
						}
					}
				}
				lfile.Close();
				rfile.Close();
			}
		}
		catch (...)
		{

		}
	}
	
	return ret;
}

ResultPtr ActionNormalizationOperator::RunCPU(ActionDataPacketPtr indata, int desiredframes, int sr, ActionRepresentationDataPacketPtr outdata, std::atomic<bool>& cancel)
{
	ResultPtr ret = std::make_shared<Result>(false);

	std::wstring mode = L"So3";
	try
	{
		mode = params->GetItem<std::wstring>(L"mode");
	}
	catch (...)
	{
		Logger::Get().Report(LogLevel::Info) << "mode changed to default \"So3\"" << Logger::endl;
	}

	std::wstring submode = L"save";
	try
	{
		submode = params->GetItem<std::wstring>(L"submode");
	}
	catch (...)
	{
		Logger::Get().Report(LogLevel::Info) << "submode changed to default \"save\"" << Logger::endl;
	}

	std::shared_ptr<So3LieAlgebra> so3;
	if (mode.compare(L"So3") == 0)
	{
		so3 = So3LieAlgebra::Create(params, ret);
		if (!ret->State)
		{
			return ret;
		}
	}

	try
	{
		int finalfeaturecount = 0;
		if (so3) finalfeaturecount += so3->GetOutputFeatureCount();

		if(submode.compare(L"save") == 0)
		{
			std::wstring samplesoutput;
			try
			{
				samplesoutput = params->GetItem<std::wstring>(L"samplesoutput");
			}
			catch (...)
			{

			}

			if (finalfeaturecount)
			{
				FileStream outfile;
				bool hasoutfile = false;
				int seqrecorded = 0;
				if (!samplesoutput.empty())
				{
					hasoutfile = outfile.Open(samplesoutput.c_str(), FileAccess::Write, FileShare::Write, FileMode::Create);
					outfile.Write((char*)&desiredframes, sizeof(int), 0, sizeof(int));
					outfile.Write((char*)&finalfeaturecount, sizeof(int), 0, sizeof(int));
					outfile.Write((char*)&seqrecorded, sizeof(int), 0, sizeof(int));
				}

				size_t seqcount = indata->Sequences->size();
				size_t seqcounter = 0;
				for (size_t si = 0; si < seqcount; si++)
				{
					ActionDataSequencePtr inseqptr = indata->Sequences->at(si);
					int iframecount = inseqptr->FrameCount;
					if (iframecount <= 0) continue;
					if (!inseqptr->Joints) continue;
					int jointcount = inseqptr->Joints->Shape()[1];
					if (jointcount != ACTION_JOINTCOUNT) continue;
					auto jointsbuf = inseqptr->Joints.get();
					auto joints = inseqptr->Joints->GetCpu();

					std::shared_ptr<GenericBuffer<Point3>> normjointsbuf = std::make_shared<GenericBuffer<Point3>>(desiredframes, ACTION_JOINTCOUNT);
					Point3* normjoints = normjointsbuf->GetCpu();

					/*interpolation*/
					/*normalization*/
					for (int j = 0; j < ACTION_JOINTCOUNT; j++)
					{
						Trajectory traj{};
						for (int fi = 0; fi < iframecount; fi++)
						{
							auto pt = (joints + jointsbuf->Index(fi, 0))[j];
							if (has_nan(pt))
								traj.Push((datatype)fi / sr, -1);
							else
							{
								traj.Push((datatype)fi / sr, -1, pt);
							}
						}

						if (Interpolator::Process(&traj))
						{
							if (Interpolator::Resample(&traj, desiredframes))
							{
								Point3 pt3;
								int lab = -1;
								datatype ptime;

								for (unsigned int i = 0; i < desiredframes; i++)
								{
									if (traj.GetPointNorm(pt3, ptime, lab, i))
									{
										(normjoints + ACTION_JOINTCOUNT* i)[j] = pt3;
									}
									else
									{
										(normjoints + ACTION_JOINTCOUNT* i)[j].setConstant(NAN);
									}
								}
							}
						}
					}

					/*scaling*/
					std::vector<double> bases;
					for (int di = 0; di < desiredframes; di++)
					{
						auto subjnorms = normjoints + ACTION_JOINTCOUNT* di;
						for (int j = 0; j < ACTION_JOINTNORMALNODE; j++)
						{
							subjnorms[ACTION_JOINTNORMAL[j]] = subjnorms[ACTION_JOINTNORMAL[j]] - subjnorms[ACTION_JOINTROOT];
						}
						if (!has_nan(subjnorms[0]) && !has_nan(subjnorms[20]))
						{
							bases.push_back((double)sqrt((subjnorms[0] - subjnorms[20]).squaredNorm()));
						}
					}

					float avgbase = (float)(std::accumulate(bases.begin(), bases.end(), 0.0) / bases.size());
					for (int di = 0; di < desiredframes; di++)
					{
						auto subjnorms = normjoints + ACTION_JOINTCOUNT* di;
						for (int j = 0; j < ACTION_JOINTNORMALNODE; j++)
						{
							if (!has_nan(subjnorms[j])) subjnorms[j] = subjnorms[j] / avgbase;
						}
					}

					std::vector<std::vector<datatype>> features;
					features.resize(desiredframes);

					if (so3) so3->Compute(normjointsbuf, features);

					if (hasoutfile)
					{
						outfile.Write((char*)&inseqptr->ActionLabel, sizeof(int), 0, sizeof(int));
						for (int di = 0; di < desiredframes; di++)
						{
							outfile.Write((char*)features[di].data(), sizeof(datatype)* finalfeaturecount, 0, sizeof(datatype)* finalfeaturecount);
						}
						seqrecorded++;
						outfile.Flush();
					}

					seqcounter++;
					Logger::Get().Report(LogLevel::Info) << seqcounter << " / " << seqcount << " sequences are finished" << Logger::endl;
				}

				if (hasoutfile)
				{
					outfile.Flush();
					outfile.Seek(sizeof(int)*2, SeekOrigin::Begin);
					outfile.Write((char*)&seqrecorded, sizeof(int), 0, sizeof(int));
					outfile.Flush();
					outfile.Close();
					Logger::Get().Report(LogLevel::Info) << "samples are saved in " << samplesoutput.c_str() << Logger::endl;
				}
			}
		}
		ret->State = true;
	}
	catch (const std::exception& excep)
	{
		ret->Error = Converter::Convert(excep.what());
	}
	return ret;
}

ResultPtr ActionNormalizationOperator::Run(ActionDataPacketPtr indata, data::ParameterPacketPtr inparam, _type intype, data::ParameterPacketPtr param, ActionRepresentationDataPacketPtr outdata, std::atomic<bool>& cancel)
{
	ResultPtr ret = std::make_shared<Result>(false);

	int desired_frames;
	int samplerate;
	try
	{
		desired_frames = params->GetItem<int>(L"desired_frames");
	}
	catch (const boost::bad_any_cast&)
	{
		ret->Message = L"invalid parameter: desired_frames";
		return ret;
	}

	try
	{
		samplerate = params->GetItem<int>(L"samplerate");
	}
	catch (const boost::bad_any_cast&)
	{
		ret->Message = L"invalid parameter: samplerate";
		return ret;
	}

	if (Runtime::Mode() == ComputationMode::CPU)
	{
		ret = RunCPU(indata, desired_frames, samplerate, outdata, cancel);
	}
	else
	{
		throw std::exception("gpu mode is not implemented");
	}

	return ret;
}
