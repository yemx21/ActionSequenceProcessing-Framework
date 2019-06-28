#include "STRUCTDataLayer.h"
#include "ActionDataPacket.h"
#include <boost\algorithm\string.hpp>
#include <filesystem>
using namespace rsdn;
using namespace rsdn::learning;
using namespace rsdn::learning::timeseries;

STRUCTDataLayer::STRUCTDataLayer()
{
	packet = std::make_shared<ActionDataPacket>();
}

STRUCTDataLayer::~STRUCTDataLayer()
{

}

bool STRUCTDataLayer::Open(const std::wstring& path)
{
	std::experimental::filesystem::path pathinfo = path;
	if (!std::experimental::filesystem::exists(pathinfo)) return false;
	if (pathinfo.extension().compare(".bsdb") != 0) return false;
	if (!bsdb->Load(path, [](const std::wstring& head, int ver)
	{
		return boost::trim_copy(head).compare(L"STRUCT") == 0 && ver > -1;
	})) return false;

	return true;
LOAD_ERR:
	bsdb->Close();
	return false;
}

ResultPtr STRUCTDataLayer::IsSupportConnectTo(_type next)
{
	bool ret = next->IsBaseOf(_typeof(TimeSeriesLayer));
	return ret ? std::make_shared<Result>(true) : std::make_shared<Result>(false, L"next layer is not a TimeSeriesLayer");
}

ResultPtr STRUCTDataLayer::ReadyCore()
{
	return bsdb->IsOpen() ? std::make_shared<Result>(true) : std::make_shared<Result>(false, L"data is not loaded");
}

ResultPtr STRUCTDataLayer::OutCore(data::DataPacketPtr& data, data::ParameterPacketPtr& parameter, _type next)
{
	data = packet;
	parameter = nullptr;
	return std::make_shared<Result>(true);
}

std::unique_ptr<std::future<ResultPtr>> STRUCTDataLayer::RunAsync(std::atomic<bool>& cancel)
{
	return std::make_unique<std::future<ResultPtr>>(std::async(std::launch::deferred, [&cancel, &datapacket = packet, &reader = bsdb]()
	{
		ResultPtr ret = std::make_shared<Result>(false);
		try
		{
			if (!reader->Seek(0)) { ret->Message = L"seek operation error"; return ret; }

			BinaryBufferPtr tmpbuf = std::make_shared<BinaryBuffer>();

			int seqcounter = 0;

			std::shared_ptr<GenericBuffer<Point3>> nanptsbuf = std::make_shared<GenericBuffer<Point3>>(STRUCT_JOINTCOUNT);
			Point3* nanpts = nanptsbuf->GetCpu();
			for (int i = 0; i < STRUCT_JOINTCOUNT; i++) nanpts[i] = Point3{ NAN, NAN, NAN };

			while (!reader->IsEOF())
			{
				if (cancel) { ret->Message = L"operation cancelled"; break; }
				if (reader->Next(tmpbuf))
				{
					auto cpubuf = tmpbuf->GetCpuReadonlyBuffer();
					ActionDataSequencePtr seq = std::make_shared<ActionDataSequence>();
					seq->Id = seqcounter;

					int setupid;
					int replicationnum;

					cpubuf->Read<int>(&setupid, 1);
					cpubuf->Read<int>(&seq->ViewId, 1);
					cpubuf->Read<int>(&seq->Subject, 1);
					cpubuf->Read<int>(&replicationnum, 1);
					cpubuf->Read<int>(&seq->ActionLabel, 1);

					int framecount = 0;
					cpubuf->Read<int>(&framecount, 1);
					seq->FrameCount = framecount;

					bool hasdata = false;
					cpubuf->Read<bool>(&hasdata, 1);

					if (hasdata)
					{
						bool skip = false;
						seq->Joints->Reshape(framecount, STRUCT_JOINTCOUNT);
						auto jointcpu = seq->Joints->GetCpu();
						for (int fi = 0; fi < framecount; fi++)
						{
							bool hasbody = false;
							cpubuf->Read<bool>(&hasbody, 1);
							auto cjoint = jointcpu + fi * STRUCT_JOINTCOUNT;
							if (hasbody)
							{
								int jointcount = STRUCT_JOINTCOUNT;
								cpubuf->Read<int>(&jointcount, 1);
								if (jointcount != 25)
								{
									skip = true;
									break;
								}
								else
								{
									cpubuf->Read<Point3>(cjoint, STRUCT_JOINTCOUNT);
								}
							}
							else
							{
								memcpy(cjoint, nanpts, sizeof(Point3) * STRUCT_JOINTCOUNT);
							}
						}
						if (!skip)
						{
							seqcounter++;
							datapacket->Sequences->push_back(seq);
						}
					}
					/*debug*/
					//break;
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
