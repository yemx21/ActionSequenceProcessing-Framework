#pragma once
#include "ActionNormalizationOperator.h"
namespace rsdn
{
	namespace learning
	{
		namespace timeseries
		{
			namespace details
			{
				class So3LieAlgebra
				{
				private:
					std::shared_ptr<GenericBuffer<int>> bone_joints1;
					std::shared_ptr<GenericBuffer<int>> bone_joints2;

					const int* bonejoint1a;
					const int* bonejoint1b;

					const int* bonejoint2a;
					const int* bonejoint2b;

					int anglecount;
				public:
					__forceinline Vector4 vrrotvec(const Vector3& v1, const Vector3& v2, const float eps = 10e-12f);

					__forceinline Matrix3 vrrotvec2mat(const Vector4& v, float eps = 10e-12f);

					__forceinline Vector4 vrrotmat2vec(const Matrix3& v, float eps = 10e-12f);

					__forceinline Vector3 log_map_so3_rm(const Point3& p1, const Point3& p2, float eps = 10e-12f);

					__forceinline Vector3 log_map_so3_rm(const Matrix3& p1, const Matrix3& p2, float eps = 10e-12f);

					__forceinline Matrix3 exp_map_so3_rm(const Matrix3& p, const Vector3& d, float eps = 10e-12f);

					__forceinline std::shared_ptr<GenericBuffer<Matrix3>> CalculateLocalJointAngleCPU(const Point3* joints);
				
					So3LieAlgebra();

					static std::shared_ptr<So3LieAlgebra> Create(data::ParameterPacketPtr params, ResultPtr ret);
				
					int GetOutputFeatureCount();

					void Compute(std::shared_ptr<GenericBuffer<Point3>> seq, std::vector<std::vector<datatype>>& features);
				};
			}
		}
	}
}