#pragma once
#include "Human_Config.h"

#define GAITCYCLE_CHANNEL 12

namespace rsdn
{
	namespace learning
	{
		namespace timeseries
		{
			class Floor;
			typedef std::shared_ptr<Floor> FloorPtr;

			namespace details
			{
				class GaitFrame
				{
				public:
					int Phase;
					int Event;
					datatype Time;
					std::shared_ptr<GenericBuffer<Point3>> Joints;

					GaitFrame(const int ph);
					GaitFrame(const GaitFrame& gf);
				};

				enum class GaitCycleNormalizationMethod
				{
					Start,
					End,
				};

				class GaitCycleDurationRegr
				{
				public:
					virtual void AddPoints(datatype x, datatype y);
					virtual datatype PredictY(datatype x);
				};
				typedef std::shared_ptr<GaitCycleDurationRegr> GaitCycleDurationRegrPtr;

				class GaitCycle
				{
				public:
					static int GetPhase(int evt1, int evt2);
					static GaitCycleDurationRegrPtr CreateGaitCycleDurationRegr(const std::vector<std::pair<datatype, datatype>>& data);

					bool Complete;
					int StandardizeSize;
					datatype Duration;
					datatype Velocity;
					datatype StartTime;
					datatype EndTime;
					std::vector<GaitFrame> Frames;
					GaitCycleNormalizationMethod Method;
					std::vector<GaitFrame> StandardizedFrames;

					BufferPtr FinalKinematics;
					BufferPtr FinalTimes;
					std::shared_ptr<GenericBuffer<int>> FinalLabels;

					GaitCycle();

					void Standardize(int osr, int jointnum, int length = 100);
					bool UpdateVelocity();

					bool ComputeKinematics(FloorPtr floor);

					void CopyKinematics(datatype* kinematics, datatype* times, int* labels, int ch);

					static void ComputeKinematics(const std::vector<GaitFrame>& frames, FloorPtr floor, BufferPtr kinematics, BufferPtr times, std::shared_ptr<GenericBuffer<int>> labels);
				};
			}
		}
	}
}
