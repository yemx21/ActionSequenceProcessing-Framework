#include "GaitPhaseRepresentationOperator.h"
#include "Interpolator.h"
#include "GaitCycle.h"
#include <algorithm>
#include <numeric>
#include <ostream>
#include <fstream>

using namespace rsdn;
using namespace rsdn::learning;
using namespace rsdn::learning::timeseries;
using namespace rsdn::learning::timeseries::operators;
using namespace rsdn::learning::timeseries::details;

static Optional<int> GaitPhaseRepresentationOperator_FindClosestIndex(const datatype* times, int count, datatype target, datatype acceptedrange = 1000.0f / 120.0f)
{
	Optional<int> closest;

	datatype minDifference = std::numeric_limits<datatype>::infinity();
	datatype rightcut = target + acceptedrange;
	for (int i = 0; i < count; i++)
	{
		auto difference = abs(times[i] - target);
		if (minDifference > difference)
		{
			minDifference = difference;
			closest = i;
		}
	}
	if (minDifference > acceptedrange)
	{
		closest.Reset();
	}
	return closest;
}

static Optional<int> GaitPhaseRepresentationOperator_FuzzyLeftFindClosestIndex(const datatype* times, int count, datatype target, datatype acceptedrange= 1000.0f/120.0f)
{
	Optional<int> closest;

	datatype minDifference = std::numeric_limits<datatype>::infinity();
	datatype rightcut = target + acceptedrange;
	for (int i = 0; i < count; i++)
	{
		if (rightcut - times[i] > 0.000001)
		{
			auto difference = abs(times[i] - target);
			if (minDifference > difference)
			{
				minDifference = difference;
				closest = i;
			}
		}
	}
	return closest;
}

static Optional<int> GaitPhaseRepresentationOperator_FuzzyRightFindClosestIndex(const datatype* times, int count, datatype target, datatype acceptedrange = 1000.0f / 120.0f)
{
	Optional<int> closest;

	datatype minDifference = std::numeric_limits<datatype>::infinity();
	datatype leftcut = target - acceptedrange;
	for (int i = 0; i < count; i++)
	{
		if (times[i] - leftcut> 0.000001)
		{
			auto difference = abs(times[i] - target);
			if (minDifference > difference)
			{
				minDifference = difference;
				closest = i;
			}
		}
	}
	return closest;
}

static void GaitPhaseRepresentationOperator_Select(const std::vector<GaitCycle>& cycles, const std::vector<int>& owners, const datatype* times, int valcount, int curidx, std::vector<int>& selowners, int& newidx, int& winstart, int& winend)
{
	int nowcycidx = owners[curidx];
	const GaitCycle* prevcyc = nowcycidx > 0 ? &cycles[nowcycidx - 1] : nullptr;
	const GaitCycle* nowcyc = &cycles[nowcycidx];
	const GaitCycle* nextcyc = nowcycidx < cycles.size() - 1 ? &cycles[nowcycidx + 1] : nullptr;

	datatype current = times[curidx];

	datatype HalfDuration = 0.5f * nowcyc->Duration;

	datatype left_time0 = current - HalfDuration;
	if (left_time0 < nowcyc->StartTime && prevcyc)
	{
		left_time0 = nowcyc->StartTime - (nowcyc->StartTime - left_time0) / nowcyc->Duration * prevcyc->Duration;
	}

	datatype right_time0 = current + HalfDuration;
	if (right_time0 > nowcyc->EndTime && nextcyc)
	{
		right_time0 = nowcyc->EndTime + (right_time0 - nowcyc->EndTime) / nowcyc->Duration * nextcyc->Duration;
	}

	bool has_left_time0 = false;
	bool has_right_time0 = false;
	for (int i = 0; i < valcount; i++)
	{
		datatype val = times[i];
		if (val <= left_time0) has_left_time0 = true;
		if (val >= right_time0) has_right_time0 = true;
		if (has_left_time0 && has_right_time0) break;
	}

	if (has_left_time0 && has_right_time0)
	{
		Optional<int> startidx = GaitPhaseRepresentationOperator_FuzzyLeftFindClosestIndex(times, valcount, left_time0);
		Optional<int> endidx = GaitPhaseRepresentationOperator_FuzzyRightFindClosestIndex(times, valcount, right_time0);
		for (int i = *startidx; i <= *endidx; i++)
		{
			if (i == curidx)
			{
				newidx = selowners.size();
			}
			selowners.push_back(owners[i]);
		}
		winstart = *startidx;
		winend = *endidx;
	}
	else if (has_left_time0 && !has_right_time0)
	{
		int endidx = valcount - 1;
		Optional<int> startidx = GaitPhaseRepresentationOperator_FuzzyLeftFindClosestIndex(times, valcount, left_time0);
		for (int i = *startidx; i <= endidx; i++)
		{
			if (i == curidx)
			{
				newidx = selowners.size();
			}
			selowners.push_back(owners[i]);
		}
		winstart = *startidx;
		winend = endidx;
	}
	else if (!has_left_time0 && has_right_time0)
	{
		int startidx = 0;
		Optional<int> endidx = GaitPhaseRepresentationOperator_FuzzyRightFindClosestIndex(times, valcount, right_time0);
		for (int i = startidx; i <= *endidx; i++)
		{
			if (i == curidx)
			{
				newidx = selowners.size();
			}
			selowners.push_back(owners[i]);
		}
		winstart = startidx;
		winend = *endidx;
	}
	else
	{
		winstart = curidx;
		winend = curidx;
	}

}

constexpr datatype GaitPhaseRepresentation_PositiveMaskValue = 2.0 / 100.0;
constexpr datatype GaitPhaseRepresentation_NegativeMaskValue = -2.0 / 100.0;

static void GaitPhaseRepresentationOperator_CalculateFeatures(const std::vector<GaitCycle>& cycles, const datatype* features, const datatype* times, int valcount, const std::vector<int>& selowners, int& newidx, int winstart, int winend, std::shared_ptr<GenericBuffer<std::pair<datatype, datatype>>> pairs, datatype* outfeatures, int& nonempty)
{
	auto paircpu = pairs->GetCpu();
	int pairnum = pairs->Shape()[0];

	int nowcycidx = selowners[newidx];
	const GaitCycle* prevcyc = nowcycidx > 0 ? &cycles[nowcycidx - 1] : nullptr;
	const GaitCycle* nowcyc = &cycles[nowcycidx];
	const GaitCycle* nextcyc = nowcycidx < cycles.size() - 1 ? &cycles[nowcycidx + 1] : nullptr;
	double current = times[newidx];

	for(int i = 0; i < pairnum; i++)
	{
		const auto& cpair = paircpu[i];

		double utime = current + cpair.first * nowcyc->Duration;
		if (utime < nowcyc->StartTime && prevcyc)
		{
			utime = nowcyc->StartTime - (nowcyc->StartTime - utime) / nowcyc->Duration * prevcyc->Duration;
		}
		else if (utime > nowcyc->EndTime && nextcyc)
		{
			utime = nowcyc->EndTime + (utime - nowcyc->EndTime) / nowcyc->Duration * nextcyc->Duration;
		}

		double vtime = current + cpair.second * nowcyc->Duration;
		if (vtime < nowcyc->StartTime && prevcyc)
		{
			vtime = nowcyc->StartTime - (nowcyc->StartTime - vtime) / nowcyc->Duration * prevcyc->Duration;
		}
		else if (utime > nowcyc->EndTime && nextcyc)
		{
			vtime = nowcyc->EndTime + (vtime - nowcyc->EndTime) / nowcyc->Duration * nextcyc->Duration;
		}

		Optional<int> uindex = GaitPhaseRepresentationOperator_FindClosestIndex(times, valcount, utime);
		Optional<int> vindex = GaitPhaseRepresentationOperator_FindClosestIndex(times, valcount, vtime);
		if (uindex && vindex)
		{
			datatype tmp = features[*uindex] - features[*vindex];
			if (isnan(tmp) || isinf(tmp))
				outfeatures[i] = cpair.first < cpair.second ? GaitPhaseRepresentation_NegativeMaskValue : GaitPhaseRepresentation_PositiveMaskValue;
			else
			{
				outfeatures[i] = tmp;
				nonempty++;
			}
		}
		else
		{
			outfeatures[i] = cpair.first < cpair.second ? GaitPhaseRepresentation_NegativeMaskValue : GaitPhaseRepresentation_PositiveMaskValue;
		}
	}
}


ResultPtr GaitPhaseRepresentationOperator::RunCPU(SkeletonDataPacketPtr indata, const std::vector<std::shared_ptr<GenericBuffer<std::pair<datatype, datatype>>>>& pairs, int osr, int sr, std::shared_ptr<GenericBuffer<int>> labelsbuf, SkeletonRepresentationDataPacketPtr outdata, std::atomic<bool>& cancel)
{
	ResultPtr ret = std::make_shared<Result>(false);
	try
	{
		size_t seqcount = indata->Sequences->size();
		auto& samples = *outdata->SampleSection->Samples;

		auto ulabscount = labelsbuf->Shape()[0];
		auto ulabsbuf = labelsbuf->GetCpu();
		for (int i = 0; i < ulabscount; i++) samples.AddUniqueLabel(ulabsbuf[i]);

		int featcount = 0;
		for (const auto& chpair : pairs) featcount += chpair->Shape()[0];
		samples.SetFeatureCount(featcount);

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

				for (int fi = 0; fi<iframecount; fi++)
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
			for (auto fiter = frames.begin(); fiter != frames.end();)
			{
				if (fiter->Phase > 0 && fiter->Phase < 10)
					fiter++;
				else
					fiter = frames.erase(fiter);
			}

			int checknum = 0;
			int checkevt = 1;

		CHECKEVT:
			if (checknum > 18) continue;
			checknum ++;
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

			if (splitcyc.size() == 1)
			{
				if (checkevt == 1)
					checkevt = 9;
				else
					checkevt--;
				goto CHECKEVT;
			}

			std::vector<GaitCycle> cycles{};
			std::vector<std::pair<datatype, datatype>> cycdurs{};
			if (splitcyc[0] > 0)
			{
				GaitCycle cyc{};
				for (int i = 0; i <= splitcyc[0]; i++)
				{
					cyc.Frames.push_back(frames[i]);
				}
				cyc.StartTime = frames[0].Time;
				cyc.EndTime = frames[splitcyc[0]].Time;
				cyc.Complete = false;
				cyc.Method = GaitCycleNormalizationMethod::End;
				cycles.push_back(cyc);
			}

			int sidx = splitcyc[0];
			for (int m = 1; m < splitcyc.size(); m++)
			{
				int eidx = splitcyc[m];
				GaitCycle cyc{};
				for (int i = sidx; i <= eidx; i++)
				{
					cyc.Frames.push_back(frames[i]);
				}
				cyc.StartTime = frames[sidx].Time;
				cyc.EndTime = frames[eidx].Time;
				cyc.Complete = true;
				cyc.Duration = cyc.Frames.back().Time - cyc.Frames.front().Time;
				if (cyc.Duration > 400)
				{
					if (cyc.UpdateVelocity())
					{
						cycdurs.push_back(std::make_pair(cyc.Velocity, cyc.Duration));
					}
				}
				cycles.push_back(cyc);
				sidx = eidx;
			}

			if (sidx != frames.size() - 1)
			{
				GaitCycle cyc{};
				for (int i = sidx; i < frames.size(); i++)
				{
					cyc.Frames.push_back(frames[i]);
				}
				cyc.StartTime = frames[sidx].Time;
				cyc.EndTime = frames[frames.size() - 1].Time;
				cyc.Complete = false;
				cycles.push_back(cyc);
			}

			auto regr = GaitCycle::CreateGaitCycleDurationRegr(cycdurs);

			auto averdur = std::accumulate(cycdurs.begin(), cycdurs.end(), 0.0, [](double sum, const std::pair<datatype, datatype>& elem) {return sum + elem.second; }) / cycdurs.size();

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

			std::vector<int> owners{};
			for(const auto& fr : frames)
			{
				int findex = -1;
				for (int i = 0; i < cycles.size(); i++)
				{
					const auto& cyc = cycles.at(i);
					if (fr.Time >= cyc.StartTime && fr.Time <= cyc.EndTime)
					{
						findex = i;
						break;
					}
				}
				owners.push_back(findex);
			}

			BufferPtr kinematicsbuf = std::make_shared<Buffer>();
			BufferPtr timesbuf = std::make_shared<Buffer>();
			std::shared_ptr<GenericBuffer<int>> labelsbuf = std::make_shared<GenericBuffer<int>>();
			GaitCycle::ComputeKinematics(frames, inseqptr->Floor, kinematicsbuf, timesbuf, labelsbuf);



			int frnum = kinematicsbuf->Shape()[1];
			int jobcount = Runtime::CpuJobCount();
			BufferPtr tmpfeatbuf = std::make_shared<Buffer>(featcount);
			datatype* tmpfeatcpu = tmpfeatbuf->GetCpu();
			int* labelscpu = labelsbuf->GetCpu();
			BufferPtr tmpfeatsbuf= std::make_shared<Buffer>(jobcount, featcount);
			auto tmpfeats = tmpfeatbuf->GetCpu();
			std::vector<std::vector<datatype*>> chfeats;
			for (int j = 0; j < jobcount; j++)
			{
				std::vector<datatype*> chtmpfeats;
				datatype* chtmpfeatptr = tmpfeatbuf->GetCpu() + j* featcount;
				int chfeatstart = 0;
				for (int ch = 0; ch < GAITCYCLE_CHANNEL; ch++)
				{
					chtmpfeats.push_back(chtmpfeatptr + chfeatstart);
					chfeatstart += pairs[ch]->Shape()[0];
				}
				chfeats.push_back(chtmpfeats);
			}

			std::mutex samplelock;

			//#pragma omp parallel for num_threads(jobcount)
			for (int fr = 0; fr < frnum; fr++)
			{
				int threadid = omp_get_thread_num();
				const std::vector<datatype*>& tmpfeat = chfeats[threadid];
				int nonempty = 0;
				for (int ch = 0; ch < GAITCYCLE_CHANNEL; ch++)
				{
					const datatype* features = kinematicsbuf->GetCpu() + ch* frnum;
					const datatype* times = timesbuf->GetCpu();

					std::vector<int> selowners;
					int newselidx = -1;
					int winstart = 0;
					int winend = frnum - 1;
					GaitPhaseRepresentationOperator_Select(cycles, owners, times, frnum, fr, selowners, newselidx, winstart, winend);
					GaitPhaseRepresentationOperator_CalculateFeatures(cycles, features + winstart, times + winstart, winend-winstart, selowners, newselidx, winstart, winend, pairs[ch], tmpfeat[ch], nonempty);
				}

				if (nonempty)
				{
					{
						std::lock_guard<std::mutex> locker(samplelock);
						outdata->SampleSection->Samples->PushWithLabel(tmpfeatbuf->GetCpu() + threadid * featcount, labelscpu[fr]);
					}
				}
			}
			Logger::Get().Report(LogLevel::Info) << si + 1 << L" sequences finished" << Logger::endl;
		}

		outdata->SetSection(L"samples", outdata->SampleSection);

		ret->State = true;

		std::wstring samplesoutput;
		try
		{
			samplesoutput = params->GetItem<std::wstring>(L"samplesoutput");
			ret->State = outdata->SampleSection->Samples->Save(samplesoutput);
		}
		catch (...) { ret->State = false; }
	}
	catch (const std::exception& excep)
	{
		ret->Error = Converter::Convert(excep.what());
	}
	return ret;
}

ResultPtr GaitPhaseRepresentationOperator::ConfigCore(data::ParameterPacketPtr param)
{
	params = param;
	return std::make_shared<Result>(true);
}

ResultPtr GaitPhaseRepresentationOperator::Run(SkeletonDataPacketPtr indata, data::ParameterPacketPtr inparam, _type intype, data::ParameterPacketPtr param, SkeletonRepresentationDataPacketPtr outdata, std::atomic<bool>& cancel)
{
	ResultPtr ret = std::make_shared<Result>(false);

	std::wstring featinput;
	int rawsamplerate;
	int resamplerate;

	try
	{
		featinput = params->GetItem<std::wstring>(L"featinput");
	}
	catch (const boost::bad_any_cast&)
	{
		ret->Message = L"invalid parameter: featinput";
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

	std::shared_ptr<GenericBuffer<int>> labelsbuf;
	try
	{
		labelsbuf = std::dynamic_pointer_cast<GenericBuffer<int>>(params->GetItemEx(L"labels"));
	}
	catch (...)
	{
		ret->Message = L"invalid parameter: labels";
		return ret;
	}

	std::vector<std::shared_ptr<GenericBuffer<std::pair<datatype, datatype>>>> feats;

	try
	{
		BSDBReaderPtr reader = std::make_shared<BSDBReader>();
		if (reader->Load(featinput, [](const std::wstring& header, int) { return header.compare(L"rsdn::learning::timeseries::operators::periodicpairwisefeatures=>minedfeaturepairs")==0; }))
		{
			auto label_distfunbuf = reader->operator[](L"label_distfun");
			int label_distfun = 0;
			label_distfunbuf->GetCpuReadonlyBuffer()->Read(&label_distfun, 1);

			auto featurecountbuf = reader->operator[](L"featurecount");
			int featurecount = 0;
			featurecountbuf->GetCpuReadonlyBuffer()->Read(&featurecount, 1);

			int chnum = (int)reader->ChunkCount();
			for (int ch = 0; ch < chnum; ch++)
			{
				auto featbuf = std::make_shared<GenericBuffer<std::pair<datatype, datatype>>>();
				BinaryBufferPtr tmpfeatbuf = std::make_shared<BinaryBuffer>();
				reader->Read(tmpfeatbuf, ch);
				featbuf->Reshape(tmpfeatbuf->Shape()/ sizeof(std::pair<datatype, datatype>));
				memcpy(featbuf->GetCpu(), tmpfeatbuf->GetCpu(), tmpfeatbuf->Shape());
				feats.push_back(featbuf);
			}
			reader->Close();
		}
		else
		{
			ret->State = false;
			ret->Message = L"can not load feature file: invalid header information";
			return ret;
		}
	}
	catch (...)
	{
		ret->Message = L"can not load feature file";
		return ret;
	}

	if (Runtime::Mode() == ComputationMode::CPU)
	{
		ret = RunCPU(indata, feats, rawsamplerate, resamplerate, labelsbuf, outdata, cancel);
	}
	else
	{
		throw std::exception("gpu mode is not implemented");
	}

	return ret;
}