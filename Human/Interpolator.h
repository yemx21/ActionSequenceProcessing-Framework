#pragma once
#include "Human_Config.h"

namespace rsdn
{
	namespace learning
	{
		namespace timeseries
		{
			namespace details
			{
				class Trajectory
				{
				public:
					struct Point3Cache
					{
					public:
						int phase;
						datatype time;
						Point3 point;
						bool need;

						Point3Cache();

						Point3Cache(datatype t, int ph);

						Point3Cache(datatype t, int ph, const Point3& pt);

						Point3Cache(const Point3Cache& b);

						bool operator<(const Point3Cache& b)const;
					};

					Optional<datatype> first_timeStamp;

					std::vector<Point3Cache> data;

					size_t keyframe;

					Trajectory();

					void Push(datatype timeStamp, int ph, const Point3& pt);

					void Push(datatype timeStamp, int ph);

					void SetFirstKeyFrame(size_t index);

					unsigned int GetCount() const;

					bool GetPoint(Point3& pt, datatype& timeidx, int& ph, unsigned int index);

					void Clear();
				};

				class Interpolator
				{
				public:
					static bool Process(Trajectory* target);

					static bool Resample(Trajectory* target, unsigned int sr_src, unsigned int sr_dest);

					static bool Smooth(Trajectory* target);
				};
			}
		}
	}
}