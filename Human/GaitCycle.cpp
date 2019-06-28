#include "GaitCycle.h"
#include "SkeletonDataPacket.h"
#include "Interpolator.h"
using namespace rsdn;
using namespace rsdn::learning::timeseries;
using namespace rsdn::learning::timeseries::details;

GaitFrame::GaitFrame(int ph) :Phase(ph), Event(0)
{
	Joints = std::make_shared<GenericBuffer<Point3>>();
}

GaitFrame::GaitFrame(const GaitFrame& gf): Phase(gf.Phase), Time(gf.Time), Event(gf.Event)
{
	Joints = gf.Joints;
}

int GaitCycle::GetPhase(int evt1, int evt2)
{
	int ret = 0;
	if (evt1 == 1 && evt2 == 2)
	{
		ret = 1;
	}
	else if (evt1 == 2 && evt2 == 3)
	{
		ret = 2;
	}
	else if (evt1 == 3 && evt2 == 4)
	{
		ret = 3;
	}
	else if (evt1 == 4 && evt2 == 5)
	{
		ret = 4;
	}
	else if (evt1 == 5 && evt2 == 6)
	{
		ret = 5;
	}
	else if (evt1 == 6 && evt2 == 7)
	{
		ret = 6;
	}
	else if (evt1 == 7 && evt2 == 8)
	{
		ret = 7;
	}
	else if (evt1 == 8 && evt2 == 9)
	{
		ret = 8;
	}
	else if (evt1 == 9 && evt2 == 1)
	{
		ret = 9;
	}
	return ret;
}

GaitCycle::GaitCycle(): Complete(false), StandardizeSize(0), Duration(NAN), Method(GaitCycleNormalizationMethod::Start), StartTime(NAN), EndTime(NAN)
{
	FinalKinematics = std::make_shared<Buffer>();
	FinalTimes = std::make_shared<Buffer>();
	FinalLabels = std::make_shared<GenericBuffer<int>>();
}

void GaitCycle::Standardize(int osr, int jointnum, int length)
{
	StandardizeSize = length;
	double stime = 0.0;
	if (Method == GaitCycleNormalizationMethod::Start)
	{
		stime = Frames.front().Time;
	}
	else if (Method == GaitCycleNormalizationMethod::End)
	{
		stime = Frames.back().Time - Duration;
	}
	else
		throw std::exception("standardize error");

	std::vector<GaitFrame> tmpFrames = Frames;
	std::vector<datatype> newtimes;
	datatype interval = Duration / (datatype)length;
	for (int i = 0; i < length; i++) newtimes.push_back(stime + i * interval);
	
	for (auto titer = newtimes.begin(); titer != newtimes.end();)
	{
		if ((*titer) < 0.0 || (*titer) - stime > Duration)
			titer = newtimes.erase(titer);
		else
			titer++;
	}

	datatype acceptinterval = 1000.0 / osr;
	for(datatype t : newtimes)
	{ 
		int nearest_tfi = -1;
		datatype nearest_tdiff = std::numeric_limits<datatype>::max();
		int tfrnum = (int)tmpFrames.size();
		for (int tfi = 0; tfi < tfrnum; tfi++)
		{
			datatype ntdif = abs(tmpFrames[tfi].Time - t);
			if (ntdif < acceptinterval)
			{
				if (ntdif < nearest_tdiff)
				{
					nearest_tdiff = ntdif;
					nearest_tfi = tfi;
				}
			}
		}
		if (nearest_tfi != -1)
		{
			const auto& togf = tmpFrames[nearest_tfi];
			GaitFrame tngf{ togf.Phase };
			tngf.Event = togf.Event;
			tngf.Time = t;
			tngf.Joints = std::make_shared<GenericBuffer<Point3>>(togf.Joints->Shape()[0]);
			memcpy(tngf.Joints->GetCpu(), togf.Joints->GetCpu(), sizeof(Point3)* togf.Joints->Shape()[0]);
			tmpFrames.push_back(tngf);
		}
	}
	std::sort(tmpFrames.begin(), tmpFrames.end(), [](const GaitFrame&a, const GaitFrame& b)
	{
		return a.Time < b.Time;
	});

	for (int j = 0; j < jointnum; j++)
	{
		Trajectory traj{};
		int tmpframecount = tmpFrames.size();
		for (int fi = 0; fi<tmpframecount; fi++)
		{
			const auto& tfr = tmpFrames[fi];
			auto pt = tfr.Joints->GetCpu()[j];
			if (has_nan(pt))
				traj.Push(tfr.Time, tfr.Phase);
			else
				traj.Push(tfr.Time, tfr.Phase, pt);
		}

		if (Interpolator::Process(&traj))
		{
			Interpolator::Smooth(&traj);
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

					if (tmpFrames.size() > i)
					{
						GaitFrame& fd = tmpFrames.at(i);
						fd.Phase = ph;
						fd.Time = ptime;
						fd.Joints->GetCpu()[j] = pt3;
					}
					else
					{
						GaitFrame fd{ ph };
						fd.Time = ptime;
						fd.Joints->Reshape(jointnum);
						auto fdjointcpu = fd.Joints->GetCpu();
						fdjointcpu[j] = pt3;
						for (int n = 0; n < jointnum; n++)
						{
							if (n != j) fdjointcpu[n].fill(NAN);
						}
						tmpFrames.push_back(fd);
					}
				}
			}
		}
	}

	StandardizedFrames.clear();
	datatype acceptinterval1 = acceptinterval * 2;
	for(datatype nt : newtimes)
	{
		int nearest_tfi1 = -1;
		datatype nearest_tdiff1 = std::numeric_limits<datatype>::max();
		int tfrnum = (int)tmpFrames.size();
		for (int tfi = 0; tfi < tfrnum; tfi++)
		{
			datatype ntdif = abs(tmpFrames[tfi].Time - nt);
			if (ntdif < acceptinterval1)
			{
				if (ntdif < nearest_tdiff1)
				{
					nearest_tdiff1 = ntdif;
					nearest_tfi1 = tfi;
				}
			}
		}
		if (nearest_tfi1 != -1)
		{
			const GaitFrame& tfr1 = tmpFrames[nearest_tfi1];
			GaitFrame nfr{ tfr1.Phase };
			nfr.Event = tfr1.Event;
			nfr.Time = std::max((tfr1.Time - stime) / Duration, 0.0);
			nfr.Joints = tfr1.Joints;
			StandardizedFrames.push_back(nfr);
		}
		else
		{
			if (!Complete)
			{
				GaitFrame nfr{ -1 };
				nfr.Event = -1;
				nfr.Time = (nt - stime) / Duration;
				StandardizedFrames.push_back(nfr);
			}
			else
				throw std::exception("standardization frame error");
		}
	}

	if (StandardizedFrames.size() > 1)
	{
		for (auto sfiter = StandardizedFrames.begin(); sfiter != StandardizedFrames.end();)
		{
			if (sfiter->Time < 0.f || sfiter->Time > 1.f)
				sfiter = StandardizedFrames.erase(sfiter);
			else
				sfiter++;
		}
		StandardizedFrames.front().Phase = 1;
		StandardizedFrames.back().Phase = 9;
	}
}

bool GaitCycle::UpdateVelocity()
{
	int framecount = Frames.size();
	Optional<Point3> startpt;
	Optional<datatype> starttime;
	for (int i = 0; i < framecount; i++)
	{
		Point3 pt = Frames[i].Joints->GetCpu()[0];
		if (!has_nan(pt))
		{
			startpt = pt;
			starttime = Frames[i].Time;
			break;
		}
	}
	Optional<Point3> endpt;
	Optional<datatype> endtime;
	for (int i = framecount-1; i >=0; i--)
	{
		Point3 pt = Frames[i].Joints->GetCpu()[0];
		if (!has_nan(pt))
		{
			endpt = pt;
			endtime = Frames[i].Time;
			break;
		}
	}

	if (startpt && endpt && starttime && endtime)
	{
		auto velocity = sqrt(((*endpt) - (*startpt)).squaredNorm());
		Velocity = velocity / (*endtime - *starttime);
		return !isnan(velocity) && !isinf(velocity);
	}
	return false;
}

void GaitCycleDurationRegr::AddPoints(datatype x, datatype y)
{

}

datatype GaitCycleDurationRegr::PredictY(datatype x)
{
	return NAN;
}

namespace rsdn
{
	namespace learning
	{
		namespace timeseries
		{
			namespace details
			{
				class GaitCycleDurationRegr1 : public GaitCycleDurationRegr
				{
				private:
					datatype _x;
					datatype _y;
				public:
					void AddPoints(datatype x, datatype y) override
					{
						_x = x;
						_y = y;
					}

					datatype PredictY(datatype x) override
					{
						return x * _y / _x;
					}
				};

				class GaitCycleDurationRegr2 : public GaitCycleDurationRegr
				{
				private:
					std::vector<datatype> _x;
					std::vector<datatype> _y;

				public:
					void AddPoints(datatype x, datatype y) override
					{
						_x.push_back(x);
						_y.push_back(y);
					}

					datatype PredictY(datatype x)  override
					{
						datatype ksum = 0;
						for (size_t i = 0; i < _x.size(); i++)
						{
							ksum += (_y[i] / _x[i]);
						}
						return x * ksum / (datatype)_x.size();
					}
				};
			}
		}
	}
}

GaitCycleDurationRegrPtr GaitCycle::CreateGaitCycleDurationRegr(const std::vector<std::pair<datatype, datatype>>& data)
{
	GaitCycleDurationRegrPtr regr;
	if (data.size() == 0)
	{
		regr = nullptr;
	}
	else if (data.size() == 1)
	{
		regr = std::make_shared<GaitCycleDurationRegr1>();
	}
	else
	{
		regr = std::make_shared<GaitCycleDurationRegr2>();
	}

	if (regr)
	{
		for(const auto& pd: data)
		{
			regr->AddPoints(pd.first, pd.second);
		}
	}

	return regr;
}

namespace rsdn
{
	namespace learning
	{
		namespace timeseries
		{
			namespace details
			{
				/*
				Hip=0,
				Knee=1,
				Ankle=2,
				Toe=3,
				Heel=4,
				Toe1=5,
				Heel1=6
				*/
				#pragma region FeatureCandidate

				template<int ID>
				class FeatureCandidate
				{
				};

				template<>
				class FeatureCandidate<1>
				{
				public:
					static constexpr int Id = 1;
					inline static datatype Compute(FloorPtr floor, const GaitFrame& gf)
					{
						if (gf.Joints)
						{
							auto joints = gf.Joints->GetCpu();
							Point3 hip = joints[0];
							Point3 toe = joints[3];
							Point3 heel = joints[4];
							Point3 toe1 = joints[5];
							Point3 heel1 = joints[6];

							Point3 foot = (toe + heel) * 0.5f;
							Point3 foot1 = (toe1 + heel1) * 0.5f;

							auto proj = floor->ProjectFrom(foot);
							auto proj1 = floor->ProjectFrom(foot1);
							auto hipdist = floor->DistanceFrom(hip, false);
							return sqrt(pow(proj.x() - proj1.x(), 2.f) + pow(proj.z() - proj1.z(), 2.f)) / hipdist;
						}
						return NAN;
					}
				};

				template<>
				class FeatureCandidate<2>
				{
				public:
					static constexpr int Id = 2;
					inline static datatype Compute(FloorPtr floor, const GaitFrame& gf)
					{
						if (gf.Joints)
						{
							auto joints = gf.Joints->GetCpu();
							Point3 hip = joints[0];
							Point3 knee = joints[1];
							Point3 ankle = joints[2];
							return AngleCalculator::Process(hip, knee, ankle, true, true) / 180.0;
						}
						return NAN;
					}
				};

				template<>
				class FeatureCandidate<3>
				{
				public:
					static constexpr int Id = 3;
					inline static datatype Compute(FloorPtr floor, const GaitFrame& gf)
					{
						if (gf.Joints)
						{
							auto joints = gf.Joints->GetCpu();
							Point3 hip = joints[0];
							Point3 knee = joints[1];

							Point3 project_hip = floor->ProjectFrom(hip);

							return AngleCalculator::Process(project_hip, hip, knee, false, true) / 180.0;
						}
						return NAN;
					}
				};

				template<>
				class FeatureCandidate<4>
				{
				public:
					static constexpr int Id = 4;
					inline static datatype Compute(FloorPtr floor, const GaitFrame& gf)
					{
						if (gf.Joints)
						{
							auto joints = gf.Joints->GetCpu();
							Point3 hip = joints[0];
							Point3 toe = joints[3];

							datatype hip_dist = floor->DistanceFrom(hip, false);
							datatype toe_dist = floor->DistanceFrom(toe, true);
							if (toe_dist > 0.0) toe_dist = 0.0;
							return toe_dist / hip_dist;
						}
						return NAN;
					}
				};

				template<>
				class FeatureCandidate<5>
				{
				public:
					static constexpr int Id = 5;
					inline static datatype Compute(FloorPtr floor, const GaitFrame& gf)
					{
						if (gf.Joints)
						{
							auto joints = gf.Joints->GetCpu();
							Point3 hip = joints[0];
							Point3 heel = joints[4];

							datatype hip_dist = floor->DistanceFrom(hip, false);
							datatype heel_dist = floor->DistanceFrom(heel, true);
							if (heel_dist > 0.0) heel_dist = 0.0;
							return heel_dist / hip_dist;
						}
						return NAN;
					}
				};

				template<>
				class FeatureCandidate<6>
				{
				public:
					static constexpr int Id = 6;
					inline static datatype Compute(FloorPtr floor, const GaitFrame& gf)
					{
						if (gf.Joints)
						{
							auto joints = gf.Joints->GetCpu();
							Point3 hip = joints[0];
							Point3 toe1 = joints[5];

							datatype hip_dist = floor->DistanceFrom(hip, false);
							datatype toe1_dist = floor->DistanceFrom(toe1, true);
							if (toe1_dist > 0.0) toe1_dist = 0.0;
							return toe1_dist / hip_dist;
						}
						return NAN;
					}
				};

				template<>
				class FeatureCandidate<7>
				{
				public:
					static constexpr int Id = 7;
					inline static datatype Compute(FloorPtr floor, const GaitFrame& gf)
					{
						if (gf.Joints)
						{
							auto joints = gf.Joints->GetCpu();
							Point3 hip = joints[0];
							Point3 heel1 = joints[6];

							datatype hip_dist = floor->DistanceFrom(hip, false);
							datatype heel1_dist = floor->DistanceFrom(heel1, true);
							if (heel1_dist > 0.0) heel1_dist = 0.0;
							return heel1_dist / hip_dist;
						}
						return NAN;
					}
				};

				template<>
				class FeatureCandidate<8>
				{
				public:
					static constexpr int Id = 8;
					inline static datatype Compute(FloorPtr floor, const GaitFrame& gf)
					{
						if (gf.Joints)
						{
							auto joints = gf.Joints->GetCpu();
							Point3 hip = joints[0];
							Point3 toe1 = joints[5];
							Point3 heel1 = joints[6];

							Point3 project_hip = floor->ProjectFrom(hip);
							Point3 foot1 = (toe1 + heel1) * 0.5;
							return AngleCalculator::Process(project_hip, hip, foot1, false, true) / 180.0;
						}
						return NAN;
					}
				};

				template<>
				class FeatureCandidate<9>
				{
				public:
					static constexpr int Id = 9;
					inline static datatype Compute(FloorPtr floor, const GaitFrame& gf)
					{
						if (gf.Joints)
						{
							auto joints = gf.Joints->GetCpu();
							Point3 knee = joints[1];
							Point3 ankle = joints[2];
							Point3 toe = joints[3];

							return AngleCalculator::Process(knee, ankle, toe, false, true) / 180.0;
						}
						return NAN;
					}
				};

				template<>
				class FeatureCandidate<10>
				{
				public:
					static constexpr int Id = 10;
					inline static datatype Compute(FloorPtr floor, const GaitFrame& gf)
					{
						if (gf.Joints)
						{
							auto joints = gf.Joints->GetCpu();
							Point3 knee = joints[1];
							Point3 ankle = joints[2];

							Point3 anklefloor = floor->IntersectPointBy(knee, ankle);
							Point3 kneeproj = floor->ProjectFrom(knee);

							return AngleCalculator::Process(knee, anklefloor, kneeproj, false, true) / 180.0;
						}
						return NAN;
					}
				};

				template<>
				class FeatureCandidate<11>
				{
				public:
					static constexpr int Id = 11;
					inline static datatype Compute(FloorPtr floor, const GaitFrame& gf)
					{
						if (gf.Joints)
						{
							auto joints = gf.Joints->GetCpu();
							Point3 toe = joints[3];
							Point3 heel = joints[4];

							Point3 toefloor = floor->IntersectPointBy(heel, toe);
							Point3 heelproj = floor->ProjectFrom(heel);
							return AngleCalculator::Process(heel, toefloor, heelproj, false, false) / 180.0;
						}
						return NAN;
					}
				};

				template<>
				class FeatureCandidate<12>
				{
				public:
					static constexpr int Id = 12;
					inline static datatype Compute(FloorPtr floor, const GaitFrame& gf)
					{
						if (gf.Joints)
						{
							auto joints = gf.Joints->GetCpu();
							Point3 toe1 = joints[5];
							Point3 heel1 = joints[6];

							Point3 toe1floor = floor->IntersectPointBy(heel1, toe1);
							Point3 heel1proj = floor->ProjectFrom(heel1);
							return AngleCalculator::Process(heel1, toe1floor, heel1proj, false, false) / 180.0;
						}
						return NAN;
					}
				};

				#pragma endregion

				class FeatureCandidates
				{
				public:
					static datatype Compute(FloorPtr floor, const GaitFrame& gf, int ch)
					{
						if (ch == 0) return FeatureCandidate<1>::Compute(floor, gf);
						if (ch == 1) return FeatureCandidate<2>::Compute(floor, gf);
						if (ch == 2) return FeatureCandidate<3>::Compute(floor, gf);
						if (ch == 3) return FeatureCandidate<4>::Compute(floor, gf);
						if (ch == 4) return FeatureCandidate<5>::Compute(floor, gf);
						if (ch == 5) return FeatureCandidate<6>::Compute(floor, gf);
						if (ch == 6) return FeatureCandidate<7>::Compute(floor, gf);
						if (ch == 7) return FeatureCandidate<8>::Compute(floor, gf);
						if (ch == 8) return FeatureCandidate<9>::Compute(floor, gf);
						if (ch == 9) return FeatureCandidate<10>::Compute(floor, gf);
						if (ch == 10) return FeatureCandidate<11>::Compute(floor, gf);
						if (ch == 11) return FeatureCandidate<12>::Compute(floor, gf);
						return NAN;
					}
				};
			}
		}
	}
}

bool GaitCycle::ComputeKinematics(FloorPtr floor)
{
	int sfcount = (int)StandardizedFrames.size();

	BufferPtr tmpkinematicsbuf = std::make_shared<Buffer>(GAITCYCLE_CHANNEL, sfcount);
	datatype* tmpkinematics = tmpkinematicsbuf->GetCpu();

	for (int ch = 0; ch < GAITCYCLE_CHANNEL; ch++)
	{
		for (int fi = 0; fi < sfcount; fi++)
		{
			tmpkinematics[fi]= FeatureCandidates::Compute(floor, StandardizedFrames[fi], ch);
		}
		tmpkinematics += sfcount;
	}

	tmpkinematics = tmpkinematicsbuf->GetCpu();
	std::vector<int> valids;
	for (int fi = 0; fi < sfcount; fi++)
	{
		bool allreal = true;
		for (int ch = 0; ch < GAITCYCLE_CHANNEL; ch++)
		{
			if (isnan(tmpkinematics[tmpkinematicsbuf->Index(ch, fi)])) 
			{
				allreal = false;
				break;
			}
		}
		if (allreal)
		{
			valids.push_back(fi);
		}
	}

	int totalnum = (int)valids.size();
	FinalKinematics->Reshape(GAITCYCLE_CHANNEL, totalnum);
	FinalTimes->Reshape(totalnum);
	FinalLabels->Reshape(totalnum);
	datatype* kinematics = FinalKinematics->GetCpu();
	datatype* times = FinalTimes->GetCpu();
	int* labels = FinalLabels->GetCpu();

	for (int ch = 0; ch < GAITCYCLE_CHANNEL; ch++)
	{
		for (int vi = 0; vi < totalnum; vi++)
		{
			kinematics[vi] = tmpkinematics[tmpkinematicsbuf->Index(ch, valids[vi])];
		}
		kinematics += totalnum;
	}

	for (int vi = 0; vi < totalnum; vi++)
	{
		times[vi] = StandardizedFrames[valids[vi]].Time;
		labels[vi] = StandardizedFrames[valids[vi]].Phase;
	}

	Complete = totalnum == StandardizeSize;

	return true;
}

void GaitCycle::CopyKinematics(datatype* kinematics, datatype* times, int* labels, int ch)
{
	datatype* fkinematics = FinalKinematics->GetCpu() + FinalKinematics->Index(ch, 0);
	datatype* ftimes = FinalTimes->GetCpu();
	int* flabels = FinalLabels->GetCpu();

	memcpy(kinematics, fkinematics, sizeof(datatype) * StandardizeSize);
	memcpy(times, ftimes, sizeof(datatype) * StandardizeSize);
	memcpy(labels, flabels, sizeof(int) * StandardizeSize);
}

void GaitCycle::ComputeKinematics(const std::vector<GaitFrame>& frames, FloorPtr floor, BufferPtr kinematics, BufferPtr times, std::shared_ptr<GenericBuffer<int>> labels)
{
	int frnum = (int)frames.size();
	kinematics->Reshape(GAITCYCLE_CHANNEL, frnum);
	times->Reshape(frnum);
	labels->Reshape(frnum);
	datatype* tmpkinematics = kinematics->GetCpu();
	datatype* timecpu = times->GetCpu();
	int* labelcpu = labels->GetCpu();

	for (int ch = 0; ch < GAITCYCLE_CHANNEL; ch++)
	{
		//#pragma omp parallel for num_threads(Runtime::CpuJobCount())
		for (int fi = 0; fi < frnum; fi++)
		{
			tmpkinematics[fi] = FeatureCandidates::Compute(floor, frames[fi], ch);
		}
		tmpkinematics += frnum;
	}
	for (int fi = 0; fi < frnum; fi++)
	{
		timecpu[fi] = frames[fi].Time;
		labelcpu[fi] = frames[fi].Phase;
	}
}