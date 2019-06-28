#include "SkeletonDataLayer.h"
#include <boost\algorithm\string.hpp>
#include <filesystem>
#include "SkeletonDataPacket.h"
using namespace rsdn;
using namespace rsdn::learning::timeseries;

SkeletonDataLayer::SkeletonDataLayer()
{
	packet = std::make_shared<SkeletonDataPacket>();
}


SkeletonDataLayer::~SkeletonDataLayer()
{

}

bool SkeletonDataLayer::Open(const std::wstring& path)
{
	std::experimental::filesystem::path pathinfo = path;
	if (!std::experimental::filesystem::exists(pathinfo)) return false;
	if (pathinfo.extension().compare(".bsdb") != 0) return false;
	if (!bsdb->Load(path, [](const std::wstring& head, int ver)
	{
		return boost::trim_copy(head).compare(L"SKEL")==0 && ver > -1;
	})) return false;
		
	/*read skeleton model*/
	auto skelmol = bsdb->operator[](L"skeletonmodel");
	if(skelmol)
	{
		auto modbuf = skelmol->GetCpuReadonlyBuffer();
		int jointcount = 0;
		modbuf->Read<int>(&jointcount, 1);
		for (int i = 0; i < jointcount; i++)
		{
			int jointid = -1;
			modbuf->Read<int>(&jointid, 1);
			auto str = modbuf->ReadString();
			boost::trim(str);
			if (str.empty()) goto LOAD_ERR;
			packet->Model->Add(std::make_shared<JointNode>(jointid, str));
		}
		auto rootcount = 0;
		modbuf->Read<int>(&rootcount, 1);
		for (int i = 0; i < rootcount; i++)
		{
			packet->Model->AddRoot(packet->Model->Joints[i]);
		}
		auto groupcount = 0;
		modbuf->Read<int>(&groupcount, 1);
		for (int i = 0; i < groupcount; i++)
		{
			auto parentid = -1; 
			modbuf->Read<int>(&parentid, 1);
			auto parent = packet->Model->Joints[parentid];

			auto childcount = 0;
			modbuf->Read<int>(&childcount, 1);
			for (int j = 0; j < groupcount; j++)
			{
				int id = 0;
				modbuf->Read<int>(&id, 1);
				packet->Model->AddChild(parent, packet->Model->Joints[id]);
			}
		}
		return true;
	}

LOAD_ERR:
	bsdb->Close();
	return false;
}

ResultPtr SkeletonDataLayer::IsSupportConnectTo(_type next)
{
	bool ret = next->IsBaseOf(_typeof(TimeSeriesLayer));
	return ret? std::make_shared<Result>(true): std::make_shared<Result>(false, L"next layer is not a TimeSeriesLayer");
}

ResultPtr SkeletonDataLayer::ReadyCore()
{
	return bsdb->IsOpen() ? std::make_shared<Result>(true) : std::make_shared<Result>(false, L"data is not loaded");
}

ResultPtr SkeletonDataLayer::OutCore(data::DataPacketPtr& data, data::ParameterPacketPtr& parameter, _type next)
{
	data = packet;
	parameter = nullptr;
	return std::make_shared<Result>(true);
}

std::unique_ptr<std::future<ResultPtr>> SkeletonDataLayer::RunAsync(std::atomic<bool>& cancel)
{
	return std::make_unique<std::future<ResultPtr>>(std::async(std::launch::deferred, [&cancel, &datapacket=packet, &reader=bsdb]()
	{ 
		ResultPtr ret = std::make_shared<Result>(false);
		try
		{
			if (!reader->Seek(0)) { ret->Message = L"seek operation error"; return ret; }

			BinaryBufferPtr tmpbuf = std::make_shared<BinaryBuffer>();

			while (!reader->IsEOF())
			{
				if (cancel) { ret->Message = L"operation cancelled"; break; }
				if (reader->Next(tmpbuf))
				{
					auto cpubuf = tmpbuf->GetCpuReadonlyBuffer();
					SkeletonDataSequencePtr seq = std::make_shared<SkeletonDataSequence>();
					cpubuf->Read<__int64>(&seq->Id, 1);
					cpubuf->Read<__int64>(&seq->Subject, 1);

					int regjointcount = 0;
					cpubuf->Read<int>(&regjointcount, 1);
					if (regjointcount)
					{
						seq->JointIds->Reshape(regjointcount);
						cpubuf->Read<int>(seq->JointIds->GetCpu(), regjointcount);
					}

					bool hasfloor = false;
					cpubuf->Read<bool>(&hasfloor, 1);
					if (hasfloor)
					{
						float floora, floorb, floorc, floord = 1.0f;
						cpubuf->Read<float>(&floora, 1);
						cpubuf->Read<float>(&floorb, 1);
						cpubuf->Read<float>(&floorc, 1);
						cpubuf->Read<float>(&floord, 1);
						seq->Floor = std::make_shared<Floor>(floora, floorb, floorc, floord);
					}

					cpubuf->Read<unsigned __int64>(&seq->FrameCount, 1);
					if (seq->FrameCount)
					{
						cpubuf->ReadBuffer(seq->Times, seq->FrameCount);
						int jointcount = datapacket->Model->Count;
						seq->Joints->Reshape(seq->FrameCount, jointcount);
						cpubuf->Read<Point3>(seq->Joints->GetCpu(), seq->FrameCount * jointcount);

						bool haslabels = false;
						cpubuf->Read<bool>(&haslabels, 1);
						if (haslabels)
						{
							seq->Labels->Reshape(seq->FrameCount);
							cpubuf->Read<int>(seq->Labels->GetCpu(), seq->FrameCount);
						}

						bool hasevents = false;
						cpubuf->Read<bool>(&hasevents, 1);
						if (hasevents)
						{
							int eventnum = 0;
							cpubuf->Read<int>(&eventnum, 1);
							seq->EventTimes->Reshape(eventnum);
							cpubuf->Read(seq->EventTimes->GetCpu(), eventnum);

							seq->Events->Reshape(eventnum);
							cpubuf->Read<int>(seq->Events->GetCpu(), eventnum);
						}
					}

					datapacket->Sequences->push_back(seq);
				}
				else
				{
					break;
				}
			}

			ret->State = true;
		}
		catch (const std::exception& excep)
		{
			ret->Error = Converter::Convert(excep.what());
		}
		return ret;
	}));
}
