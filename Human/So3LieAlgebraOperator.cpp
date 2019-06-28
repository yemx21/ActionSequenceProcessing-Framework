#include "So3LieAlgebraOperator.h"

using namespace rsdn;
using namespace rsdn::learning;
using namespace rsdn::learning::timeseries;
using namespace rsdn::learning::timeseries::operators;

Vector4 So3LieAlgebraOperator::vrrotvec(const Vector3& v1, const Vector3& v2, const float eps)
{
	Vector3 an = v1.normalized();
	Vector3 bn = v1.normalized();

	if (an.isZero(eps)) an.setZero();
	if (bn.isZero(eps)) bn.setZero();

	Vector3 ax = an.cross(bn).normalized();
	if (ax.isZero(eps)) ax.setZero();

	float dotabn = an.dot(bn);

	float angle = acos(dotabn < 1.0f ? dotabn : 1.0f);

	if (ax.nonZeros()==0)
	{
		Vector3 absa = an.cwiseAbs();

		Vector3 c { 0.f, 0.f, 0.f };
		int minidx = 0;
		float mincoeff = c.minCoeff<int>(&minidx);
		c.coeffRef(mincoeff) = 1.0f;
		
		ax = an.cross(c).normalized();
		if (ax.isZero(eps)) ax.setZero();
	}

	return Vector4{ ax.x(), ax.y(), ax.z(), angle };
}

Matrix3 So3LieAlgebraOperator::vrrotvec2mat(const Vector4& v, const float eps)
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

Vector4 So3LieAlgebraOperator::vrrotmat2vec(const Matrix3& v, const float eps)
{
	auto mtrc = v.trace();
	if (abs(mtrc - 3) <= eps)
	{
		return Vector4{ 0.f, 1.f, 0.f, 0.f };
	}
	else if (abs(mtrc + 1) <= eps)
	{
		Vector3 axis0 = sqrt(((v.diagonal().transpose() + Vector3::Ones()) / 2).cwiseMax(Vector3{ 0.f,0.f,0.f }));
		auto axis = axis0.unaryExpr([&eps](float& elem)
		{
			if (elem < eps) elem = 0.0f;
		});

		Vector3 upper{ v(2,3), v(1,3), v(1,2) };

		auto signs= upper.unaryExpr([&eps](float& elem)
		{
			elem = (abs(elem) < eps ? 0.f : (elem > 0.f ? 1.f : -1.f)) * (abs(elem) > eps ? 1.f : 0.f);
		});

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
			Vector3 shifted{ signs(3), signs(1), signs(2) };
			flip = shifted.unaryExpr([&eps](float& elem)
			{
				elem = elem + (abs(elem) < eps ? 1.f : 0.f);
			});
		}

		auto axis2 = axis * flip;
		Vector4 r{ axis2.x(), axis2.y(), axis2.z(), (const float)M_PI };

		constexpr float thr = 10e-5;
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
						throw std::exception("could not find an appropriate value");
					}
				}
			}
		}
		return r;
	}
	
	auto phi = acos((mtrc - 1) / 2);
	auto den = 2 * sin(phi);
	auto axis = Vector3{ v(3, 2) - v(2, 3), v(1, 3) - v(3, 1), v(2, 1) - v(1, 2) }/ den;
	return Vector4{ axis.x(), axis.y(), axis.z(), phi };
}

Vector3 So3LieAlgebraOperator::log_map_so3_rm(const Point3& p1, const Point3& p2, const float eps)
{
	auto axis_angle = vrrotmat2vec(p2 * p1.transpose());

	return axis_angle.head<3>() * axis_angle(4);
}

Vector3 So3LieAlgebraOperator::log_map_so3_rm(const Matrix3& p1, const Matrix3& p2, const float eps)
{
	auto axis_angle = vrrotmat2vec(p2 * p1.transpose());

	return axis_angle.head<3>() * axis_angle(4);
}

Matrix3 So3LieAlgebraOperator::exp_map_so3_rm(const Matrix3& p, const Vector3& d, const float eps)
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

ResultPtr So3LieAlgebraOperator::Run(SkeletonDataPacketPtr indata, data::ParameterPacketPtr inparam, _type intype, data::ParameterPacketPtr param,
	SkeletonRepresentationDataPacketPtr outdata, std::atomic<bool>& cancel)
{
	ResultPtr ret = std::make_shared<Result>(false);

	/*load bone_joints 1&2*/
	std::shared_ptr<GenericBuffer<int>> bone_joints1;
	std::shared_ptr<GenericBuffer<int>> bone_joints2;
	int desired_frames;
	try
	{
		bone_joints1 = boost::any_cast<std::shared_ptr<GenericBuffer<int>>>(param->GetItem(L"angle_bone1_joints"));
	}
	catch (const boost::bad_any_cast&)
	{
		ret->Message = L"invalid parameter: angle_bone1_joints";
		return ret;
	}

	try
	{
		bone_joints2 = boost::any_cast<std::shared_ptr<GenericBuffer<int>>>(param->GetItem(L"angle_bone2_joints"));
	}
	catch (const boost::bad_any_cast&)
	{
		ret->Message = L"invalid parameter: angle_bone2_joints";
		return ret;
	}

	if (!bone_joints1->IsShapeSameAs(bone_joints2))
	{
		ret->Message = L"invalid parameter: length of angle_bone1_joints and angle_bone2_joints must be equal";
		return ret;
	}

	try
	{
		desired_frames = boost::any_cast<int>(param->GetItem(L"desired_frames"));
	}
	catch (const boost::bad_any_cast&)
	{
		ret->Message = L"invalid parameter: angle_bone2_joints";
		return ret;
	}

	if (Runtime::Mode() == ComputationMode::CPU)
	{
		ret = RunCPU(indata, desired_frames, bone_joints1, bone_joints2, outdata, cancel);
	}
	else
	{
		throw std::exception("gpu mode is not implemented");
	}

	return ret;
}

std::shared_ptr<GenericBuffer<Matrix3>> So3LieAlgebraOperator::CalculateLocalJointAngleCPU(const Point3* joints, int anglecount, const int* bonejoint1a, const int* bonejoint1b, const int* bonejoint2a, const int* bonejoint2b)
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
			bone1_global = Vector3{ 1.0, 1.0, 1.0 } - joints[bonejoint1a[i]];

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

void So3LieAlgebraOperator::resample_so3(const concurrency::concurrent_vector<std::shared_ptr<GenericBuffer<Matrix3>>>& framewise, std::shared_ptr<GenericBuffer<Matrix3>> features, int ai, int framenum, int desiredframes, const datatype* reftimes)
{
	GenericBuffer<bool> validitybuf{ framenum };
	GenericBuffer<int> indicesbuf{ framenum };
	bool* validity = validitybuf.GetCpu();
	int* indices = indicesbuf.GetCpu();
	auto dataptr = framewise[ai];
	auto data = dataptr->GetCpu();

	for (int i = 0; i < framenum; i++)
	{
		validity[i] = !isnan(data[i].coeff(0));
	}

	Buffer timestampbuf{ framenum };
	auto timestamps = timestampbuf.GetCpu();
	int validcount = 0;
	datatype timestamp_start;
	for (int i = 0; i < framenum; i++)
	{
		if (validity[i]) 
		{
			timestamp_start = reftimes[i]; 
			break;
		}
	}
	
	for (int i = 0; i < framenum; i++)
	{
		if (validity[i])
		{
			timestamps[validcount] = reftimes[i] - timestamp_start;
			indices[validcount] = i;
			validcount++;
		}
	}

	GenericBuffer<Vector3> velocitybuf{ validcount };
	auto velocity = velocitybuf.GetCpu();

	for (int i = 0; i < validcount-1; i++)
	{
		velocity[i] = log_map_so3_rm(data[indices[i]], data[indices[i + 1]]) / (timestamps[i + 1] - timestamps[i]);
	}

	datatype totaltime = timestamps[validcount - 1];
	datatype newtimeinterval = totaltime / (desiredframes - 1);

	BufferPtr newtimestampsbuf = std::make_shared<Buffer>(desiredframes);
	std::shared_ptr<GenericBuffer<Matrix3>> newsamplesbuf = std::make_shared<GenericBuffer<Matrix3>>(desiredframes);
	datatype* newtimestamps = newtimestampsbuf->GetCpu();
	Matrix3* newsamples = newsamplesbuf->GetCpu();

	int prev_idx = 1;
	datatype time_tillnext = timestamps[prev_idx + 1] - timestamps[prev_idx];
	newsamples[0] = data[indices[0]];
	newtimestamps[0] = 0.f;
	for (int i = 1; i < desiredframes; i++)
	{
		datatype temp_time = time_tillnext;
		int j = prev_idx + 1;
		for (; j < desiredframes; j++)
		{
			if (temp_time < newtimeinterval - 10e-10)
				temp_time = temp_time + timestamps[j + 1] - timestamps[j];
			else
				break;
		}
		prev_idx = j - 1;
		time_tillnext = temp_time - newtimeinterval;
		newtimestamps[i] = timestamps[prev_idx + 1] - time_tillnext;
		datatype time_fromprev = newtimestamps[i] - timestamps[prev_idx];

		newsamples[i] = exp_map_so3_rm(data[indices[prev_idx]], velocity[prev_idx] * time_fromprev);
	}

	GenericBuffer<Vector3> liealgebrabuf{ desiredframes };
	auto liealgebra = liealgebrabuf.GetCpu();
	for (int i = 0; i < desiredframes; i++)
	{
		liealgebra[i] = log_map_so3_rm(Matrix3::Identity(), newsamples[i]);
	}

	auto feats = features->GetCpu();
	//feats[features->Index()]
}

ResultPtr So3LieAlgebraOperator::RunCPU(SkeletonDataPacketPtr indata, int desiredframes, std::shared_ptr<GenericBuffer<int>> bonejoints1, std::shared_ptr<GenericBuffer<int>> bonejoints2, SkeletonRepresentationDataPacketPtr outdata, std::atomic<bool>& cancel)
{
	ResultPtr ret = std::make_shared<Result>(false);
	try
	{
		int anglecount = bonejoints1->Shape()[0];
		if (anglecount % 2 != 0)
		{
			ret->Message = L"invalid parameter: angle pair count is invalid";
			return ret;
		}
		anglecount = anglecount / 2;

		const int* bonejoint1a = bonejoints1->GetCpu();
		const int* bonejoint1b = bonejoint1a + anglecount;

		const int* bonejoint2a = bonejoints2->GetCpu();
		const int* bonejoint2b = bonejoint2a + anglecount;

		size_t seqcount = indata->Sequences->size();

		for (size_t si = 0; si < seqcount; si++)
		{
			SkeletonDataSequencePtr inseqptr = indata->Sequences->at(si);
			int framecount = inseqptr->FrameCount;
			if (framecount <= 0) continue;
			GenericBuffer<Point3>* joints = inseqptr->Joints.get();
			int jointcount = inseqptr->Joints->Shape()[1];
			if (!joints) continue;

			const Point3* jointscpu = joints->GetCpu();
			auto reftimes = inseqptr->Times->GetCpu();

			concurrency::concurrent_vector<std::shared_ptr<GenericBuffer<Matrix3>>> framewise_features;
			framewise_features.resize(framecount, nullptr);
			#pragma omp parallel for
			for(int fi=0; fi<framecount; fi++)
			{
				auto rotm = CalculateLocalJointAngleCPU(jointscpu + fi * jointcount, anglecount, bonejoint1a, bonejoint1b, bonejoint2a, bonejoint2b);
				framewise_features[fi] = rotm;
			}

			SkeletonDataSequencePtr outseqptr = outdata->Sequences->at(si);
			outseqptr->So3Features->Reshape(anglecount * 3, desiredframes);

			#pragma omp parallel for
			for (int ai = 0; ai < anglecount; ai++)
			{

			}

		}
	}
	catch (const std::exception& excep)
	{
		ret->Error = Converter::Convert(excep.what());
	}
	return ret;
}