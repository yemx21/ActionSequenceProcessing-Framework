#pragma once
#include "SkeletonRepresentationLayer.h"
#include "SkeletonDataPacket.h"
#include "SkeletonRepresentationDataPacket.h"
#include <concurrent_vector.h>
#include <concurrent_unordered_map.h>

namespace rsdn
{
	namespace learning
	{
		namespace timeseries
		{
			namespace operators
			{
				class So3LieAlgebraOperator : public SkeletonRepresentationLayerOperator
				{
				private:

					__forceinline Vector4 vrrotvec(const Vector3& v1, const Vector3& v2, const float eps= FLT_EPSILON);

					__forceinline Matrix3 vrrotvec2mat(const Vector4& v, const float eps = FLT_EPSILON);

					__forceinline Vector4 vrrotmat2vec(const Matrix3& v, const float eps = FLT_EPSILON);

					__forceinline Vector3 log_map_so3_rm(const Point3& p1, const Point3& p2, const float eps = FLT_EPSILON);

					__forceinline Vector3 log_map_so3_rm(const Matrix3& p1, const Matrix3& p2, const float eps = FLT_EPSILON);

					__forceinline Matrix3 exp_map_so3_rm(const Matrix3& p, const Vector3& d, const float eps = FLT_EPSILON);

					__forceinline std::shared_ptr<GenericBuffer<Matrix3>> CalculateLocalJointAngleCPU(const Point3* joints, int anglecount, const int* bonejoint1a, const int* bonejoint1b, const int* bonejoint2a, const int* bonejoint2b);

					__forceinline void resample_so3(const concurrency::concurrent_vector<std::shared_ptr<GenericBuffer<Matrix3>>>& framewise, std::shared_ptr<GenericBuffer<Matrix3>> features, int ai, int framenum, int desiredframes, const datatype* reftimes);


					ResultPtr RunCPU(SkeletonDataPacketPtr indata, int desiredframes, std::shared_ptr<GenericBuffer<int>> bonejoints1, std::shared_ptr<GenericBuffer<int>> bonejoints2, SkeletonRepresentationDataPacketPtr outdata, std::atomic<bool>& cancel);

				public:
					ResultPtr Run(SkeletonDataPacketPtr indata, data::ParameterPacketPtr inparam, _type intype, data::ParameterPacketPtr param, SkeletonRepresentationDataPacketPtr outdata, std::atomic<bool>& cancel) override final;

					REFLECT_CLASS(So3LieAlgebraOperator)
				};
			}
		}
	}
}