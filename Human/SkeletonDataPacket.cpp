#include "SkeletonDataPacket.h"
using namespace rsdn::learning::timeseries;

SkeletonDataPacket::SkeletonDataPacket()
{
	Model = std::make_shared<SkeletonModel>();
	Sequences = std::make_shared<SkeletonSequenceCollection>();
}

SkeletonDataSequence::SkeletonDataSequence(): Id(-1), Subject(-1), FrameCount(0)
{
	Times = std::make_shared<Buffer>();
	Labels = std::make_shared<GenericBuffer<int>>();
	Joints = std::make_shared<GenericBuffer<Point3>>();
	JointIds = std::make_shared<GenericBuffer<int>>();
	GaitParameters = std::make_shared<Buffer>();

	EventTimes = std::make_shared<Buffer>();
	Events = std::make_shared<GenericBuffer<int>>();
}

Floor::Floor(const datatype a, const datatype b, const datatype c, const datatype d): Normal(a,b,c), A(a),B(b),C(c),D(d)
{

}

datatype Floor::DistanceFrom(const Point3& pos3, bool issigned)
{
	return issigned ? (A * pos3.x() + B * pos3.y() + C * pos3.z() + D) * (B < 0.0 ? 1 : -1) : abs(A * pos3.x() + B * pos3.y() + C * pos3.z() + D);
}

datatype Floor::AngleWithPlane(const Point3& v1, const Point3& v2, const Point3& v3)
{
	Vector3 normal123 = (v2 - v1).cross(v3 - v1);
	normal123.normalize();

	datatype ret = AngleCalculator::AngleBetween(Normal, normal123);
	if (ret < 0.f) ret = -ret;
	if (ret > 90.f) ret = 180.f - ret;
	return ret;
}

Point3 Floor::ProjectFrom(const Point3& pt)
{
	return pt - Normal * (A * pt.x() + B * pt.y() + C * pt.z() + D);
}

Point3 Floor::IntersectPointBy(const Point3& pt1, const Point3& pt2)
{
	datatype n1, n2, n3, vpt;
	n1 = 0.0;
	n2 = 0.0;
	n3 = -D / C;
	Vector3 line = pt1 - pt2;
	line.normalize();
	vpt = line.x() * A + line.y() * B + line.z() * C;
	if (vpt == 0) return Point3{ 0.f, 0.f, 0.f };
	double t = ((n1 - pt1.x()) * A + (n2 - pt1.y()) * B + (n3 - pt1.z()) * C) / vpt;
	return pt1 + line * t;
}

GaitCycleChannel::GaitCycleChannel()
{
	Kinematics = std::make_shared<Buffer>();
	Times = std::make_shared<Buffer>();
	Labels = std::make_shared<GenericBuffer<int>>();
	Id = std::make_shared<GenericBuffer<__int64>>();
	SubId = std::make_shared<GenericBuffer<__int64>>();
	Subject = std::make_shared<GenericBuffer<__int64>>();
}

boost::any GaitCycleChannel::GetDataCore(const std::wstring& key)
{
	if (key.compare(L"timefeatures") == 0) return Kinematics;
	if (key.compare(L"times") == 0) return Times;
	if (key.compare(L"labels") == 0) return Labels;
	if (key.compare(L"ids") == 0) return Id;
	if (key.compare(L"subids") == 0) return SubId;
	if (key.compare(L"subjects") == 0) return Subject;
	return nullptr;
}

JointNode::JointNode() : id(-1), isroot(false)
{

}

JointNode::JointNode(int gid, const std::wstring& str): id(gid), name(str), isroot(false)
{

}

void JointNode::AddChild(JointNodePtr child)
{
	children.push_back(child);
}

bool JointNode::GetIsRoot() const
{
	return isroot;
}

int JointNode::GetId() const
{
	return id;
}

int JointNode::GetChildCount() const
{
	return (int)children.size();
}

JointNodePtr JointNode::operator [](int index)
{
	return children[index];
}

SkeletonModel::SkeletonModel()
{
}

void SkeletonModel::Add(JointNodePtr joint)
{
	joints.push_back(joint);
}

void SkeletonModel::AddRoot(JointNodePtr root)
{
	if (!root->isroot)
	{
		root->isroot = true;
		roots.push_back(root);
	}
}

void SkeletonModel::AddChild(JointNodePtr father, JointNodePtr child)
{
	father->AddChild(child);
}

int SkeletonModel::GetCount() const
{
	return joints.size();
}

int SkeletonModel::GetRootCount() const
{
	return roots.size();
}

JointNodePtr SkeletonModel::GetRoot(int rootindex) const
{
	//if (rootindex < 0 || rootindex> roots.size() - 1) return nullptr;
	return roots[rootindex];
}

JointNodePtr SkeletonModel::GetJoint(int index) const
{
	//if (index < 0 || index> joints.size() - 1) return nullptr;
	return joints[index];
}