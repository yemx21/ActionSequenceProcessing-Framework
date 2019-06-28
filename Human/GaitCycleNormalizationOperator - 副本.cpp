#include "GaitCycleNormalizationOperator.h"
#include "Interpolator.h"
#include "GaitCycle.h"

using namespace rsdn;
using namespace rsdn::learning;
using namespace rsdn::learning::timeseries;
using namespace rsdn::learning::timeseries::operators;
using namespace rsdn::learning::timeseries::details;


ResultPtr GaitCycleNormalizationOperator::RunCPU(SkeletonDataPacketPtr indata, int desiredframes, int osr, int sr, SkeletonRepresentationDataPacketPtr outdata, std::atomic<bool>& cancel)
{
	ResultPtr ret = std::make_shared<Result>(false);
	try
	{
		size_t seqcount = indata->Sequences->size();
		for (size_t si = 0; si < seqcount; si++)
		{
			SkeletonDataSequencePtr inseqptr = indata->Sequences->at(si);
			int framecount = inseqptr->FrameCount;
			if (framecount <= 0) continue;
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
				for (int fi = 0; fi<framecount; fi++)
				{
					auto pt = (joints + jointsbuf->Index(fi, 0))[regidx];
					if (has_nan(pt))
						traj.Push(reftimes[fi], -1);
					else
						traj.Push(reftimes[fi], -1, pt);			
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

			for (int ei = 0; ei < eventnum; ei++)
			{
				datatype nearestdiff = std::numeric_limits<datatype>::max();
				int fi_nearest = -1;

				for (int fi=0; fi<framecount; fi++)
				{
					datatype diff = abs(frames[fi].Time - eventtimes[ei]);
					if (diff < nearestdiff)
					{
						nearestdiff = diff;
						fi_nearest = fi;
					}
				}

				if (fi_nearest >= 0 && fi_nearest < framecount)
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
			std::vector<std::pair<datatype, datatype>> cycdurs{};
			if (splitcyc[0] > 0)
			{
				GaitCycle cyc{};
				for (int i = 0; i <= splitcyc[0]; i++)
				{
					cyc.Frames.push_back(frames[i]);
				}
				cyc.Complete = false;
				cyc.Method = GaitCycleNormalizationMethod::End;
				cycles.push_back(cyc);
			}
			int sidx = splitcyc[0];
			datatype averdur = 0.0;
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
				cycles.push_back(cyc);
				if (cyc.Duration > 400)
				{
					if (cyc.UpdateVelocity())
					{
						cycdurs.push_back(std::make_pair(cyc.Velocity, cyc.Duration));
						averdur += cyc.Duration;
					}
				}
				sidx = eidx;
			}

			if (sidx != frames.size() - 1)
			{
				GaitCycle cyc{};
				for (int i = sidx; i < frames.size(); i++)
				{
					cyc.Frames.push_back(frames[i]);
				}
				cyc.Complete = false;
				cycles.push_back(cyc);
			}

			auto regr = GaitCycle::CreateGaitCycleDurationRegr(cycdurs);
			averdur = averdur / (datatype)cycdurs.size();

			for(auto& gc : cycles)
			{
				if (isnan(gc.Duration))
				{
					if (gc.UpdateVelocity() && regr)
					{
						gc.Duration = regr->PredictY(gc.Velocity);
					}
					else
					{
						gc.Duration = averdur;
					}
				}
			}

			int gcseqcounter = 0;
			for (auto& gc : cycles)
			{
				gc.Standardize(osr, jointcount, desiredframes);

				GaitCycleSequencePtr gcseq = std::make_shared<GaitCycleSequence>();
				gcseq->Id = inseqptr->Id;
				gcseq->SubId = gcseqcounter++;
				gcseq->Subject = inseqptr->Subject;
				gc.ComputeKinematics(gcseq->Kinematics, gcseq->Times, inseqptr->Floor);

				if (gc.Complete)
				{
					outdata->CycleSection->Cycles->push_back(gcseq);
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

ResultPtr GaitCycleNormalizationOperator::Run(SkeletonDataPacketPtr indata, data::ParameterPacketPtr inparam, _type intype, data::ParameterPacketPtr param, SkeletonRepresentationDataPacketPtr outdata, std::atomic<bool>& cancel)
{
	ResultPtr ret = std::make_shared<Result>(false);

	/*load joint registration*/
	int desired_frames;
	int rawsamplerate;
	int resamplerate;
	try
	{
		desired_frames = boost::any_cast<int>(param->GetItem(L"desired_frames"));
	}
	catch (const boost::bad_any_cast&)
	{
		ret->Message = L"invalid parameter: desired_frames";
		return ret;
	}

	try
	{
		rawsamplerate = boost::any_cast<int>(param->GetItem(L"rawsamplerate"));
	}
	catch (const boost::bad_any_cast&)
	{
		ret->Message = L"invalid parameter: rawsamplerate";
		return ret;
	}

	try
	{
		resamplerate = boost::any_cast<int>(param->GetItem(L"resamplerate"));
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