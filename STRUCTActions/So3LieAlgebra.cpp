#include "So3LieAlgebra.h"

using namespace rsdn;
using namespace rsdn::learning;
using namespace rsdn::learning::timeseries;
using namespace rsdn::learning::timeseries::details;

Vector4 So3LieAlgebra::vrrotvec(const Vector3& v1, const Vector3& v2, float eps)
{
	Vector3 an = v1.normalized();
	Vector3 bn = v2.normalized();

	if (an.isZero(eps)) an.setZero();
	if (bn.isZero(eps)) bn.setZero();

	Vector3 ax = an.cross(bn).normalized();
	if (ax.isZero(eps)) ax.setZero();

	float dotabn = an.dot(bn);

	float angle = acos(dotabn < 1.0f ? dotabn : 1.0f);

	if (ax.nonZeros() == 0)
	{
		Vector3 absa = an.cwiseAbs();

		Vector3 c{ 0.f, 0.f, 0.f };
		int minidx = 0;
		float mincoeff = c.minCoeff<int>(&minidx);
		c.coeffRef(minidx) = 1.0f;

		ax = an.cross(c).normalized();
		if (ax.isZero(eps)) ax.setZero();
	}

	return Vector4{ ax.x(), ax.y(), ax.z(), angle };
}

Matrix3 So3LieAlgebra::vrrotvec2mat(const Vector4& v, float eps)
{
	float s = sin(v.w());
	float c = cos(v.w());
	float t = 1.0f - c;

	Vector3 n = v.head<3>().normalized();
	if (n.isZero(eps)) n.setZero();

	float x = n.x();
	float y = n.y();
	float z = n.z();

	Matrix3 m;

	m(0, 0) = t * x * x + c;
	m(0, 1) = t * x * y - s * z;
	m(0, 2) = t * x * z + s * y;
	m(1, 0) = t * x * y + s * z;
	m(1, 1) = t * y * y + c;
	m(1, 2) = t * y * z - s * x;
	m(2, 0) = t * x * z - s * y;
	m(2, 1) = t * y * z + s * x;
	m(2, 2) = t * z * z + c;

	return m;
}

Vector4 So3LieAlgebra::vrrotmat2vec(const Matrix3& v, float eps)
{
	auto mtrc = v.trace();
	if (abs(mtrc - 3) <= eps)
	{
		return Vector4{ 0.f, 1.f, 0.f, 0.f };
	}
	else if (abs(mtrc + 1) <= eps)
	{
		Vector3 axis0 = (((Vector3)(v.diagonal().transpose()) + Vector3::Ones()) / 2).cwiseMax(Vector3{ 0.f,0.f,0.f }).cwiseSqrt();
		auto axis = axis0.unaryExpr(std::function<float(float)>([&eps](float elem)->float
		{
			return elem < eps ? 0.0f : elem;
		}));

		Vector3 upper{ v(1,2), v(0,2), v(0,1) };
		auto signs = upper;
		signs = upper.unaryExpr(std::function<float(float)>([&eps](float elem)->float
		{
			return (abs(elem) < eps ? 0.f : (elem > 0.f ? 1.f : -1.f)) * (abs(elem) > eps ? 1.f : 0.f);
		}));

		Vector3 flip;
		if (signs.sum() >= 0.f)
		{
			flip.setOnes();
		}
		else if (signs.nonZeros() == 0)
		{
			flip = -signs;
		}
		else
		{
			Vector3 shifted{ signs(2), signs(0), signs(1) };
			flip = shifted.unaryExpr(std::function<float(float)>([&eps](float elem)->float
			{
				return elem + (abs(elem) < eps ? 1.f : 0.f);
			}));
		}

		auto axis2 = axis.cwiseProduct(flip);
		Vector4 r{ axis2.x(), axis2.y(), axis2.z(), (const float)M_PI };

		
		constexpr float thr = 10e-5f;
		auto t = r;
		if ((vrrotvec2mat(r) - v).norm() > thr)
		{
			r(1) = -r(1);
			if ((vrrotvec2mat(r) - v).norm() > thr)
			{
				r = t;
				r(2) = -r(2);
				if ((vrrotvec2mat(r) - v).norm() > thr)
				{
					r = t;
					r(3) = -r(3);
					if ((vrrotvec2mat(r) - v).norm() > thr)
					{
						goto FFVR;
						//throw std::exception("could not find an appropriate value");
					}
				}
			}
		}
		return r;
	}

FFVR:
	auto phi = acos((mtrc - 1) / 2);
	auto den = 2 * sin(phi);
	auto axis = Vector3{ v(2, 1) - v(1, 2), v(0, 2) - v(2, 0), v(1, 0) - v(0, 1) } / den;
	return Vector4{ axis.x(), axis.y(), axis.z(), phi };
}

Vector3 So3LieAlgebra::log_map_so3_rm(const Point3& p1, const Point3& p2, float eps)
{
	auto axis_angle = vrrotmat2vec(p2 * p1.transpose());
	return axis_angle.head<3>() * axis_angle(4);
}

Vector3 So3LieAlgebra::log_map_so3_rm(const Matrix3& p1, const Matrix3& p2, float eps)
{
	auto axis_angle = vrrotmat2vec(p2 * p1.transpose());
	return axis_angle.head<3>() * axis_angle(3);
}

Matrix3 So3LieAlgebra::exp_map_so3_rm(const Matrix3& p, const Vector3& d, float eps)
{
	auto direction = d;
	auto dnorm = direction.operatorNorm();
	if (dnorm)
	{
		auto d1 = d / dnorm;
		return vrrotvec2mat(Vector4{ d1.x(), d1.y(), d1.z(), dnorm })* p;
	}
	return p;
}

So3LieAlgebra::So3LieAlgebra()
{
}

std::shared_ptr<So3LieAlgebra> So3LieAlgebra::Create(data::ParameterPacketPtr param, ResultPtr ret)
{
	std::shared_ptr<So3LieAlgebra> so3 = std::make_shared<So3LieAlgebra>();

	/*load bone_joints 1&2*/
	try
	{
		so3->bone_joints1 = std::dynamic_pointer_cast<GenericBuffer<int>>(param->GetItemEx(L"angle_bone1_joints"));
	}
	catch (const boost::bad_any_cast&)
	{
		ret->Message = L"invalid parameter: angle_bone1_joints";
		return nullptr;
	}

	try
	{
		so3->bone_joints2 = std::dynamic_pointer_cast<GenericBuffer<int>>(param->GetItemEx(L"angle_bone2_joints"));
	}
	catch (const boost::bad_any_cast&)
	{
		ret->Message = L"invalid parameter: angle_bone2_joints";
		return nullptr;
	}

	if (!so3->bone_joints1->IsShapeSameAs(so3->bone_joints2))
	{
		ret->Message = L"invalid parameter: length of angle_bone1_joints and angle_bone2_joints must be equal";
		return nullptr;
	}
	try
	{
		so3->anglecount = so3->bone_joints1->Shape()[0];
		if (so3->anglecount % 2 != 0)
		{
			ret->Message = L"invalid parameter: angle pair count is invalid";
			return nullptr;
		}
		so3->anglecount = so3->anglecount / 2;
		so3->bonejoint1a = so3->bone_joints1->GetCpu();
		so3->bonejoint1b = so3->bonejoint1a + so3->anglecount;

		so3->bonejoint2a = so3->bone_joints2->GetCpu();
		so3->bonejoint2b = so3->bonejoint2a + so3->anglecount;

		ret->State = true;

		return so3;
	}
	catch (...)
	{
		ret->Message = L"unknown error";
	}

	return nullptr;
}

std::shared_ptr<GenericBuffer<Matrix3>> So3LieAlgebra::CalculateLocalJointAngleCPU(const Point3* joints)
{
	std::shared_ptr<GenericBuffer<Matrix3>> ret = std::make_shared<GenericBuffer<Matrix3>>((const int)anglecount);
	Matrix3* retcpu = ret->GetCpu();
	Vector3 bone1_global;
	Vector3 bone2_global;
	for (int i = 0; i < anglecount; i++)
	{
		if (bonejoint1b[i])
			bone1_global = joints[bonejoint1b[i]] - joints[bonejoint1a[i]];
		else
			bone1_global = Vector3{ 1.0, 1.0, 1.0 } -joints[bonejoint1a[i]];

		if (bonejoint2b[i])
			bone2_global = joints[bonejoint2b[i]] - joints[bonejoint2a[i]];
		else
			bone2_global = Vector3{ 1.0, 1.0, 1.0 } -joints[bonejoint2a[i]];

		if (bone1_global.isZero() || bone2_global.isZero())
			retcpu[i].fill(NAN);
		else
		{
			Matrix3 rot = vrrotvec2mat(vrrotvec(bone1_global, Vector3{ 1.f, 0.f, 0.f }));
			retcpu[i] = vrrotvec2mat(vrrotvec(rot * bone1_global, rot * bone2_global));
		}
	}
	return ret;
}

int So3LieAlgebra::GetOutputFeatureCount()
{
	return anglecount * 3;
}

void So3LieAlgebra::Compute(std::shared_ptr<GenericBuffer<Point3>> seq, std::vector<std::vector<datatype>>& features)
{
	int framecount = seq->Shape()[0];
	int jointcount = seq->Shape()[1];
	auto seqcpu = seq->GetCpu();
	std::shared_ptr<GenericBuffer<Matrix3>> tmp;

	int job = Runtime::Get().GetCpuJobCount();

	for (int i = 0; i < framecount; i++)
	{
		auto& cfr = features[i];
		Point3* joints = seqcpu + jointcount* i;
		tmp = CalculateLocalJointAngleCPU(joints);
		auto tmpcpu = tmp->GetCpu();
		std::vector<datatype> cfeats;
		cfeats.resize(anglecount * 3);
		#pragma omp parallel for num_threads(job)
		for (int j = 0; j < anglecount; j++)
		{
			if (has_nan(tmpcpu[j]))
			{
				cfeats[j * 3] = NAN;
				cfeats[j * 3 + 1] = NAN;
				cfeats[j * 3 + 2] = NAN;
			}
			else
			{
				auto feat = log_map_so3_rm(Matrix3::Identity(), tmpcpu[j]);
				cfeats[j * 3] = feat.x();
				cfeats[j * 3 + 1] = feat.y();
				cfeats[j * 3 + 2] = feat.z();
			}
		}
		std::copy(cfeats.begin(), cfeats.end(), std::back_inserter(features[i]));
	}
}
