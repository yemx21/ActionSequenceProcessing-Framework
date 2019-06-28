#include "SkeletonRepresentationLayer.h"
#include "SkeletonDataLayer.h"
#include "SkeletonDataPacket.h"
#include "SkeletonRepresentationDataPacket.h"
using namespace rsdn::learning::timeseries;

SkeletonRepresentationLayer::SkeletonRepresentationLayer(): intype(nullptr)
{
	outpacket = std::make_shared<SkeletonRepresentationDataPacket>();
}


SkeletonRepresentationLayer::~SkeletonRepresentationLayer()
{

}

std::unique_ptr<std::future<ResultPtr>> SkeletonRepresentationLayer::RunAsync(std::atomic<bool>& cancel)
{
	return std::make_unique<std::future<ResultPtr>>(std::async(std::launch::deferred, [&cancel, &ops= operators, &indata = inpacket, &inty= intype, &outdata = outpacket, &inparam= inparams, &param= params]()
	{
		ResultPtr ret = std::make_shared<Result>(false);
		try
		{
			std::shared_ptr<SkeletonDataPacket> inptr = indata.lock();
			if (!inptr) { throw std::exception{ "input packet is expired" }; }
			std::shared_ptr<ParameterPacket> inparamptr = inparam.lock(); 

			outdata->SequenceSection->Sequences = inptr->Sequences;

			for (auto op : ops)
			{
				ret = op->Run(inptr, inparamptr, inty, param, outdata, cancel);
				if (!ret) 
				{ 
					ret->Error = std::wstring(op->GetType()->Name) + L" operation failed";
					return ret;
				}
			}
			
			outdata->SetSection(L"normalized_collection", outdata->CycleSection);

			ret->State = true;
		}
		catch (const std::exception& excep)
		{
			ret->Error = Converter::Convert(excep.what());
		}
		return ret;
	}));
}

ResultPtr SkeletonRepresentationLayer::IsSupportConnectFrom(_type prev)
{
	bool ret = prev->IsType(_typeof(SkeletonDataLayer));
	return ret ? std::make_shared<Result>(true) : std::make_shared<Result>(false, L"prev layer must be SkeletonDataLayer");
}

ResultPtr SkeletonRepresentationLayer::IsSupportConnectTo(_type next)
{
	bool ret = next->IsBaseOf(_typeof(TimeSeriesLayer));
	return ret ? std::make_shared<Result>(true) : std::make_shared<Result>(false, L"next layer is not a TimeSeriesLayer");
}

ResultPtr SkeletonRepresentationLayer::ReadyCore()
{
	return !inpacket.expired()? std::make_shared<Result>(true) : std::make_shared<Result>(false, L"data in expired");
}

ResultPtr SkeletonRepresentationLayer::InCore(data::DataPacketPtr data, data::ParameterPacketPtr prevparameter, _type prev)
{
	if (!data->GetType()->IsType(_typeof(SkeletonDataPacket))) return std::make_shared<Result>(false, L"prev data packet must be SkeletonDataPacket");
	inpacket = std::dynamic_pointer_cast<SkeletonDataPacket>(data);
	return std::make_shared<Result>(true);
}

ResultPtr SkeletonRepresentationLayer::OutCore(data::DataPacketPtr& data, data::ParameterPacketPtr& parameter, _type next)
{
	data = outpacket;
	parameter = params;
	return std::make_shared<Result>(true);
}

ResultPtr SkeletonRepresentationLayer::ConfigCore(data::ParameterPacketPtr param)
{
	params = param;
	return std::make_shared<Result>(true);
}

ResultPtr SkeletonRepresentationLayer::AddOperator(OperatorPtr op)
{
	if (!op) return std::make_shared<Result>(false, L"operator is null");
	SkeletonRepresentationLayerOperatorPtr op_ptr = std::dynamic_pointer_cast<SkeletonRepresentationLayerOperator>(op);
	if (!op_ptr) return std::make_shared<Result>(false, L"operator must be a SkeletonRepresentationLayerOperator");
	operators.push_back(op_ptr);
	return std::make_shared<Result>(true);
}

SkeletonRepresentationLayerOperator::SkeletonRepresentationLayerOperator()
{

}

SkeletonRepresentationLayerOperator::~SkeletonRepresentationLayerOperator()
{

}

ResultPtr SkeletonRepresentationLayerOperator::Run(SkeletonDataPacketPtr indata, data::ParameterPacketPtr inparam, _type intype, data::ParameterPacketPtr param, SkeletonRepresentationDataPacketPtr outdata, std::atomic<bool>& cancel)
{
	return std::make_shared<Result>(false);
}
