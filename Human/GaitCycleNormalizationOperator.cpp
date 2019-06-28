#include "GaitCycleNormalizationOperator.h"
#include "Interpolator.h"
#include "GaitCycle.h"

using namespace rsdn;
using namespace rsdn::learning;
using namespace rsdn::learning::timeseries;
using namespace rsdn::learning::timeseries::operators;
using namespace rsdn::learning::timeseries::details;


ResultPtr GaitCycleNormalizationOperator::ConfigCore(data::ParameterPacketPtr param)
{
	params = param;
	return std::make_shared<Result>(true);
}

ResultPtr GaitCycleNormalizationOperator::RunCPU(SkeletonDataPacketPtr indata, int desiredframes, int osr, int sr, SkeletonRepresentationDataPacketPtr outdata, std::atomic<bool>& cancel)
{
	ResultPtr ret = std::make_shared<Result>(false);
	try
	{
		size_t seqcount = indata->Sequences->size();
		std::vector<std::vector<GaitCycle>> allcycles{};
		int gcnum = 0;
		std::vector<__int64> ids;
		std::vector<__int64> subjects;
		//for (size_t si = 0; si < 4; si++)
		for (size_t si = 0; si < seqcount; si++)
		{
			SkeletonDataSequencePtr inseqptr = indata->Sequences->at(si);
			int iframecount = inseqptr->FrameCount;
			if (iframecount <= 0) continue;
			if (!inseqptr->Joints) continue;
			if (!inseqptr->JointIds) continue;
			if (!inseqptr->Floor) continue;
			int jointcount = inseqptr->Joints->Shape()[1];
			auto jointsbuf = inseqptr->Joints.get();
			auto joints = inseqptr->Joints->GetCpu();
			const int* jointreg = inseqptr->JointIds->GetCpu();
			int jointregnum = inseqptr->JointIds->Shape()[0];
			auto reftimes = inseqptr->Times->GetCpu();

			std::vector<GaitFrame> frames;
			for (int j = 0; j < jointregnum; j++)
			{
				int regidx = jointreg[j];
				Trajectory traj{};
				for (int fi = 0; fi<iframecount; fi++)
				{
					auto pt = (joints + jointsbuf->Index(fi, 0))[regidx];
					if (has_nan(pt))
						traj.Push(reftimes[fi], -1);
					else
					{
						traj.Push(reftimes[fi], -1, pt);
					}
				}

				if (Interpolator::Process(&traj))
				{
					Interpolator::Smooth(&traj);
					if (Interpolator::Resample(&traj, osr, sr))
					{
						auto tc = traj.GetCount();
						Point3 pt3;
						datatype ptime;
						int evt = 0;
						int ph = 0;
						for (unsigned int i = 0; i < tc; i++)
						{
							if (traj.GetPoint(pt3, ptime, ph, i))
							{
								bool pushfd = false;

								if (frames.size() > i)
								{
									GaitFrame& fd = frames.at(i);
									fd.Phase = ph;
									fd.Time = ptime;
									fd.Joints->GetCpu()[j] = pt3;
								}
								else
								{
									GaitFrame fd{ ph };
									fd.Time = ptime;
									fd.Joints->Reshape(jointregnum);
									auto fdjointcpu = fd.Joints->GetCpu();
									fdjointcpu[j] = pt3;
									for (int n = 0; n < jointregnum; n++)
									{
										if (n != j) fdjointcpu[n].fill(NAN);
									}
									frames.push_back(fd);
								}
							}
						}

					}
				}
			}

			int eventnum = inseqptr->EventTimes->Shape()[0];
			auto eventtimes = inseqptr->EventTimes->GetCpu();
			auto eventids = inseqptr->Events->GetCpu();

			iframecount = (int)frames.size();

			for (int ei = 0; ei < eventnum; ei++)
			{
				datatype nearestdiff = std::numeric_limits<datatype>::max();
				int fi_nearest = -1;

				for (int fi=0; fi<iframecount; fi++)
				{
					datatype diff = abs(frames[fi].Time - eventtimes[ei]);
					if (diff < nearestdiff)
					{
						nearestdiff = diff;
						fi_nearest = fi;
					}
				}

				if (fi_nearest >= 0 && fi_nearest < iframecount)
				{
					frames[fi_nearest].Event = eventids[ei];
				}
			}

			for (int ei = 1; ei < eventnum; ei++)
			{
				datatype prevtime = eventtimes[ei - 1];
				datatype nowtime = eventtimes[ei];
				for (GaitFrame& fr : frames)
				{
					if (fr.Time >= prevtime && fr.Time < nowtime)
					{
						fr.Phase = GaitCycle::GetPhase(eventids[ei - 1], eventids[ei]);
					}
				}
			}

			int framecount = frames.size();
			for (auto fiter= frames.begin(); fiter!=frames.end();)
			{
				if (fiter->Phase > 0 && fiter->Phase < 10)
					fiter++;
				else
					fiter = frames.erase(fiter);
			}

			std::vector<int> splitcyc{};
			int nm = 0;
			framecount = frames.size();
			for (int m = 0; m < framecount; m++)
			{
				if (frames[m].Event == 1)
				{
					switch (nm)
					{
					case 0:
						splitcyc.push_back(m);
						nm++;
						break;
					case 1:
					case 2:
					case 3:
						nm++;
						break;
					}
				}
				else
					nm = 0;
			}

			if (splitcyc.size() < 2) continue;

			std::vector<GaitCycle> cycles{};
			int sidx = splitcyc[0];
			for (int m = 1; m < splitcyc.size(); m++)
			{
				int eidx = splitcyc[m];
				GaitCycle cyc{};
				for (int i = sidx; i <= eidx; i++)
				{
					cyc.Frames.push_back(frames[i]);
				}
				cyc.Complete = true;
				cyc.Duration = cyc.Frames.back().Time - cyc.Frames.front().Time;
				cyc.UpdateVelocity();
				cyc.Standardize(osr, jointcount, desiredframes);
				if (cyc.ComputeKinematics(inseqptr->Floor))
				{
					if (cyc.Complete)
					{
						cycles.push_back(cyc);
					}
				}
				sidx = eidx;
			}
			if (!cycles.empty())
			{
				gcnum += (int)cycles.size();
				allcycles.push_back(cycles);
				ids.push_back(inseqptr->Id);
				subjects.push_back(inseqptr->Subject);
			}
		}

		int cycnum = allcycles.size();
		int idnum = ids.size();
		for (int i = 0; i < GAITCYCLE_CHANNEL; i++)
		{
			GaitCycleChannelPtr gcch = std::make_shared<GaitCycleChannel>();
			gcch->Id->Reshape(gcnum);
			gcch->SubId->Reshape(gcnum);
			gcch->Subject->Reshape(gcnum);
			gcch->Kinematics->Reshape(gcnum, desiredframes);
			gcch->Times->Reshape(gcnum, desiredframes);
			gcch->Labels->Reshape(gcnum, desiredframes);

			auto idcpu = gcch->Id->GetCpu();
			auto subidcpu = gcch->SubId->GetCpu();
			auto subjectcpu = gcch->Subject->GetCpu();
			auto kimacpu = gcch->Kinematics->GetCpu();
			auto timecpu = gcch->Times->GetCpu();
			auto labcpu = gcch->Labels->GetCpu();

			size_t counter = 0;
			for (int iid = 0; iid < idnum; iid++)
			{
				__int64 cid = ids[iid];
				__int64 csubj = subjects[iid];
				auto& ggch = allcycles[iid];
				int scycnum = ggch.size();
				for (int n = 0; n < scycnum; n++)
				{
					auto& sgch = ggch[n];
					idcpu[counter] = cid;
					subidcpu[counter] = scycnum;
					subjectcpu[counter] = csubj;
					sgch.CopyKinematics(kimacpu, timecpu, labcpu, i);
					kimacpu += desiredframes;
					timecpu += desiredframes;
					labcpu += desiredframes;
					counter++;
				}
			}

			outdata->CycleSection->Channels->push_back(gcch);
		}

		ret->State = true;

		std::wstring samplesoutput;
		try
		{
			samplesoutput = params->GetItem<std::wstring>(L"samplesoutput");

			FileStream outfile;
			if(outfile.Open(samplesoutput.c_str(), FileAccess::Write, FileShare::Write, FileMode::Create))
			{
				for (int ch = 0; ch < GAITCYCLE_CHANNEL; ch++)
				{
					auto& chdata = outdata->CycleSection->Channels->at(ch);

					int samlen = chdata->Kinematics->Shape()[0];
					outfile.Write((char*)&samlen, sizeof(int), 0, sizeof(int));
					outfile.Write((char*)chdata->Kinematics->GetCpu(), sizeof(datatype)* desiredframes* samlen, 0, sizeof(datatype)* desiredframes* samlen);
					outfile.Write((char*)chdata->Times->GetCpu(), sizeof(datatype)* desiredframes* samlen, 0, sizeof(datatype)* desiredframes* samlen);
					outfile.Write((char*)chdata->Labels->GetCpu(), sizeof(int)* desiredframes* samlen, 0, sizeof(int)* desiredframes* samlen);
					outfile.Flush();
				}
				outfile.Flush();
				outfile.Close();
				Logger::Get().Report(LogLevel::Info) << "samples are saved in " << samplesoutput.c_str() << Logger::endl;
			}
		}
		catch (...)
		{
			ret->State = false;
		}
	}
	catch (const std::exception& excep)
	{
		ret->Error = Converter::Convert(excep.what());
	}
	return ret;
}

ResultPtr GaitCycleNormalizationOperator::Run(SkeletonDataPacketPtr indata, data::ParameterPacketPtr inparam, _type intype, data::ParameterPacketPtr param, SkeletonRepresentationDataPacketPtr outdata, std::atomic<bool>& cancel)
{
	ResultPtr ret = std::make_shared<Result>(false);

	int desired_frames;
	int rawsamplerate;
	int resamplerate;
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
		rawsamplerate = params->GetItem<int>(L"rawsamplerate");
	}
	catch (const boost::bad_any_cast&)
	{
		ret->Message = L"invalid parameter: rawsamplerate";
		return ret;
	}

	try
	{
		resamplerate = params->GetItem<int>(L"resamplerate");
	}
	catch (const boost::bad_any_cast&)
	{
		ret->Message = L"invalid parameter: resamplerate";
		return ret;
	}

	if (Runtime::Mode() == ComputationMode::CPU)
	{
		ret = RunCPU(indata, desired_frames, rawsamplerate, resamplerate, outdata, cancel);
	}
	else
	{
		throw std::exception("gpu mode is not implemented");
	}

	return ret;
}