#include "ActionRepresentationLayer.h"
#include "STRUCTDataLayer.h"
#include "ActionDataPacket.h"
#include "ActionRepresentationDataPacket.h"
using namespace rsdn::learning::timeseries;

ActionRepresentationLayer::ActionRepresentationLayer() : intype(nullptr)
{
	outpacket = std::make_shared<ActionRepresentationDataPacket>();
}


ActionRepresentationLayer::~ActionRepresentationLayer()
{

}

std::unique_ptr<std::future<ResultPtr>> ActionRepresentationLayer::RunAsync(std::atomic<bool>& cancel)
{
	return std::make_unique<std::future<ResultPtr>>(std::async(std::launch::deferred, [&cancel, &ops = operators, &indata = inpacket, &inty = intype, &outdata = outpacket, &inparam = inparams, &param = params]()
	{
		ResultPtr ret = std::make_shared<Result>(false);
		try
		{
			std::shared_ptr<ActionDataPacket> inptr = indata.lock();
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

unsigned int ActionRepresentationLayer::GetBatchCount()
{
	unsigned int ret = 0u;
	if (operators.size() == 1)
	{
		ret = operators[0]->GetBatchCount();
	}
	return ret;
}

std::unique_ptr<std::future<ResultPtr>> ActionRepresentationLayer::PrepareBatchAsync(std::atomic<bool>& cancel)
{
	return std::make_unique<std::future<ResultPtr>>(std::async(std::launch::deferred, [&cancel, &ops = operators, &indata = inpacket, &inty = intype, &outdata = outpacket, &inparam = inparams, &param = params]()
	{
		ResultPtr ret = std::make_shared<Result>(false);
		try
		{
			std::shared_ptr<ActionDataPacket> inptr = indata.lock();
			std::shared_ptr<ParameterPacket> inparamptr = inparam.lock();
			if(inptr) outdata->SequenceSection->Sequences = inptr->Sequences;

			if (ops.size() == 1)
			{
				ret = ops[0]->PrepareBatchAsync(inptr, inparamptr, inty, param, outdata, cancel);
				if (!ret)
				{
					ret->Error = std::wstring(ops[0]->GetType()->Name) + L" operation failed";
					return ret;
				}
			}
			else
			{
				ret->Error = L"batch layer supports single operator";
				return ret;
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

std::unique_ptr<std::future<ResultPtr>> ActionRepresentationLayer::RunBatchAsync(std::atomic<bool>& cancel, unsigned int batch)
{
	tmpbatch = batch;
	return std::make_unique<std::future<ResultPtr>>(std::async(std::launch::deferred, [&batchid = tmpbatch, &cancel, &ops = operators, &indata = inpacket, &inty = intype, &outdata = outpacket, &inparam = inparams, &param = params]()
	{
		ResultPtr ret = std::make_shared<Result>(false);
		try
		{
			std::shared_ptr<ActionDataPacket> inptr = indata.lock();
			std::shared_ptr<ParameterPacket> inparamptr = inparam.lock();
			if (inptr) outdata->SequenceSection->Sequences = inptr->Sequences;

			if (ops.size() == 1)
			{
				ret = ops[0]->RunBatchAsync(inptr, inparamptr, inty, param, outdata, cancel, batchid);
			}
		}
		catch (...) {}
		return ret;
	}));
}

ResultPtr ActionRepresentationLayer::IsSupportConnectFrom(_type prev)
{
	bool ret = prev->IsType(_typeof(DataLayer));
	return ret ? std::make_shared<Result>(true) : std::make_shared<Result>(false, L"prev layer must be ActionDataLayer");
}

ResultPtr ActionRepresentationLayer::IsSupportConnectTo(_type next)
{
	bool ret = next->IsBaseOf(_typeof(TimeSeriesLayer));
	return ret ? std::make_shared<Result>(true) : std::make_shared<Result>(false, L"next layer is not a TimeSeriesLayer");
}

ResultPtr ActionRepresentationLayer::ReadyCore()
{
	return std::make_shared<Result>(true);
	//return !inpacket.expired() ? std::make_shared<Result>(true) : std::make_shared<Result>(false, L"data in expired");
}

ResultPtr ActionRepresentationLayer::InCore(data::DataPacketPtr data, data::ParameterPacketPtr prevparameter, _type prev)
{
	if (!data->GetType()->IsType(_typeof(ActionDataPacket))) return std::make_shared<Result>(false, L"prev data packet must be ActionDataPacket");
	inpacket = std::dynamic_pointer_cast<ActionDataPacket>(data);
	return std::make_shared<Result>(true);
}

ResultPtr ActionRepresentationLayer::OutCore(data::DataPacketPtr& data, data::ParameterPacketPtr& parameter, _type next)
{
	data = outpacket;
	parameter = params;
	return std::make_shared<Result>(true);
}

ResultPtr ActionRepresentationLayer::ConfigCore(data::ParameterPacketPtr param)
{
	params = param;
	return std::make_shared<Result>(true);
}

ResultPtr ActionRepresentationLayer::AddOperator(OperatorPtr op)
{
	if (!op) return std::make_shared<Result>(false, L"operator is null");
	ActionRepresentationLayerOperatorPtr op_ptr = std::dynamic_pointer_cast<ActionRepresentationLayerOperator>(op);
	if (!op_ptr) return std::make_shared<Result>(false, L"operator must be a ActionRepresentationLayerOperator");
	operators.push_back(op_ptr);
	return std::make_shared<Result>(true);
}

ActionRepresentationLayerOperator::ActionRepresentationLayerOperator()
{

}

ActionRepresentationLayerOperator::~ActionRepresentationLayerOperator()
{

}

ResultPtr ActionRepresentationLayerOperator::Run(ActionDataPacketPtr indata, data::ParameterPacketPtr inparam, _type intype, data::ParameterPacketPtr param, ActionRepresentationDataPacketPtr outdata, std::atomic<bool>& cancel)
{
	return std::make_shared<Result>(false);
}

unsigned int ActionRepresentationLayerOperator::GetBatchCount()
{
	return 0u;
}

ResultPtr ActionRepresentationLayerOperator::PrepareBatchAsync(ActionDataPacketPtr indata, data::ParameterPacketPtr inparam, _type intype, data::ParameterPacketPtr param, ActionRepresentationDataPacketPtr outdata, std::atomic<bool>& cancel)
{
	return std::make_shared<Result>(false);
}

ResultPtr ActionRepresentationLayerOperator::RunBatchAsync(ActionDataPacketPtr indata, data::ParameterPacketPtr inparam, _type intype, data::ParameterPacketPtr param, ActionRepresentationDataPacketPtr outdata, std::atomic<bool>& cancel, unsigned int batch)
{
	return std::make_shared<Result>(false);
}
