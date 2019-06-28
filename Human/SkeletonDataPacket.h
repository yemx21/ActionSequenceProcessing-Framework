#pragma once
#include "SkeletonDataLayer.h"
namespace rsdn
{
	namespace learning
	{
		namespace timeseries
		{
			class HUMAN_API Floor
			{
			public:
				datatype A;
				datatype B;
				datatype C;
				datatype D;
				Vector3 Normal;

				Floor(const datatype a, const datatype b, const datatype c, const datatype d);

				__forceinline datatype DistanceFrom(const Point3& pos3, bool issigned = false);
				__forceinline datatype AngleWithPlane(const Point3& v1, const Point3& v2, const Point3& v3);
				__forceinline Point3 ProjectFrom(const Point3& pt);
				__forceinline Point3 IntersectPointBy(const Point3& pt1, const Point3& pt2);
			};
			typedef std::shared_ptr<Floor> FloorPtr;

			class HUMAN_API SkeletonDataSequence
			{
			public:
				std::shared_ptr<GenericBuffer<Point3>> Joints;
				std::shared_ptr<GenericBuffer<int>> JointIds;
				BufferPtr GaitParameters;
				unsigned __int64 FrameCount;
				BufferPtr Times;
				std::shared_ptr<GenericBuffer<int>> Labels;
				BufferPtr EventTimes;
				std::shared_ptr<GenericBuffer<int>> Events;
				FloorPtr Floor;
				__int64 Id;
				__int64 Subject;
				SkeletonDataSequence();
			};
			typedef std::shared_ptr<SkeletonDataSequence> SkeletonDataSequencePtr;

			class HUMAN_API GaitCycleChannel : public IDataSection
			{
			public:
				BufferPtr Kinematics;
				BufferPtr Times;
				std::shared_ptr<GenericBuffer<int>> Labels;		
				std::shared_ptr<GenericBuffer<__int64>> Id;
				std::shared_ptr<GenericBuffer<__int64>> SubId;
				std::shared_ptr<GenericBuffer<__int64>> Subject;
				GaitCycleChannel();

			protected:
				boost::any GetDataCore(const std::wstring& key) override;
			};
			typedef std::shared_ptr<GaitCycleChannel> GaitCycleChannelPtr;

			class JointNode;
			typedef std::shared_ptr<JointNode> JointNodePtr;
			class SkeletonModel;

			class HUMAN_API JointNode : boost::noncopyable
			{
			private:
				bool isroot;
				int id;
				std::wstring name;
				std::vector<JointNodePtr> children;
			protected:
				friend SkeletonModel;
				void AddChild(JointNodePtr child);
			public:
				JointNode();
				JointNode(int gid, const std::wstring& str);

				bool GetIsRoot() const;
				int GetId() const;
				int GetChildCount() const;
				JointNodePtr operator [](int index);
				_declspec(property(get = GetIsRoot)) bool IsRoot;
				_declspec(property(get = GetChildCount)) int ChildCount;
				_declspec(property(get = GetId)) int Id;
			};

			class HUMAN_API SkeletonModel : boost::noncopyable
			{
			private:
				std::vector<JointNodePtr> roots;
				std::vector<JointNodePtr> joints;
			public:
				SkeletonModel();

				void Add(JointNodePtr joint);
				void AddRoot(JointNodePtr root);
				void AddChild(JointNodePtr father, JointNodePtr child);

				int GetCount() const;
				int GetRootCount() const;
				JointNodePtr GetRoot(int rootindex) const;
				JointNodePtr GetJoint(int index) const;

				_declspec(property(get = GetCount)) int Count;
				_declspec(property(get = GetRootCount)) int RootCount; 
				_declspec(property(get = GetRoot)) JointNodePtr Roots[];
				_declspec(property(get = GetJoint)) JointNodePtr Joints[];
			};
			typedef std::shared_ptr<SkeletonModel> SkeletonModelPtr;

			typedef std::vector<SkeletonDataSequencePtr> SkeletonSequenceCollection;
			typedef std::shared_ptr<SkeletonSequenceCollection> SkeletonSequenceCollectionPtr;

			typedef std::vector<GaitCycleChannelPtr> GaitCycleChannelCollection;
			typedef std::shared_ptr<GaitCycleChannelCollection> GaitCycleChannelCollectionPtr;

			class HUMAN_API SkeletonDataPacket : public TimeSeriesDataPacket
			{
			public:
				SkeletonModelPtr Model;
				SkeletonSequenceCollectionPtr Sequences;

				SkeletonDataPacket();

				REFLECT_CLASS(SkeletonDataPacket)
			};
		}
	}
}