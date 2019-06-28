#include "ActionDataPacket.h"
using namespace rsdn::learning::timeseries;


ActionDataPacket::ActionDataPacket()
{
	Sequences = std::make_shared<ActionSequenceCollection>();
}

ActionDataSequence::ActionDataSequence() : Id(-1), Subject(-1), FrameCount(0), ActionLabel(-1)
{
	Labels = std::make_shared<GenericBuffer<int>>();	
	Joints = std::make_shared<GenericBuffer<Point3>>();
	KinematicParameters = std::make_shared<Buffer>();
}
