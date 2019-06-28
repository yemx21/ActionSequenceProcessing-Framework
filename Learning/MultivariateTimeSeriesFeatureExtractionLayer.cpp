#include "MultivariateTimeSeriesFeatureExtractionLayer.h"
#include "MultivariateTimeSeriesFeaturePacket.h"

using namespace rsdn::learning::timeseries;

MultivariateTimeSeriesFeatureExtractionLayer::MultivariateTimeSeriesFeatureExtractionLayer():intype(nullptr), usedata(true), batchmode(false), curbatch(-1)
{
	outpacket = std::make_shared<MultivariateTimeSeriesFeaturePacket>();
}


MultivariateTimeSeriesFeatureExtractionLayer::~MultivariateTimeSeriesFeatureExtractionLayer()
{

}

std::unique_ptr<std::future<ResultPtr>> MultivariateTimeSeriesFeatureExtractionLayer::RunAsync(std::atomic<bool>& cancel)
{
	return std::make_unique<std::future<ResultPtr>>(std::async(std::launch::deferred, [&use_batch = batchmode, &batchid=curbatch, &usedata_real = usedata, &cancel, &ops = operators, &indata = inpacket, &inty = intype, &outdata = outpacket, &inparam = inparams, &param = params]()
	{
		ResultPtr ret = std::make_shared<Result>(false);
		try
		{
			if (!use_batch)
			{
				if (usedata_real)
				{
					TimeSeriesDataPacketPtr inptr = indata.lock();
					if (!inptr) { throw std::exception{ "input packet is expired" }; }
					std::shared_ptr<ParameterPacket> inparamptr = inparam.lock();

					for (auto op : ops)
					{
						ret = op->Run(inptr, inparamptr, inty, param, outdata, cancel);
						if (!ret->State)
						{
							ret->Error = std::wstring(op->GetType()->Name) + L" operation failed";
							return ret;
						}
					}
					ret->State = true;
				}
				else
				{
					for (auto op : ops)
					{
						ret = op->Run(nullptr, nullptr, inty, param, outdata, cancel);
						if (!ret->State)
						{
							ret->Error = std::wstring(op->GetType()->Name) + L" operation failed";
							return ret;
						}
					}
					ret->State = true;
				}
			}
			else
			{
				if (usedata_real)
				{
					TimeSeriesDataPacketPtr inptr = indata.lock();
					if (!inptr) { throw std::exception{ "input packet is expired" }; }
					std::shared_ptr<ParameterPacket> inparamptr = inparam.lock();

					for (auto op : ops)
					{
						ret = op->RunBatch(batchid, inptr, inparamptr, inty, param, outdata, cancel);
						if (!ret->State)
						{
							ret->Error = std::wstring(op->GetType()->Name) + L" operation failed";
							return ret;
						}
					}
					ret->State = true;
				}
				else
				{
					for (auto op : ops)
					{
						ret = op->RunBatch(batchid, nullptr, nullptr, inty, param, outdata, cancel);
						if (!ret->State)
						{
							ret->Error = std::wstring(op->GetType()->Name) + L" operation failed";
							return ret;
						}
					}
					ret->State = true;
				}
			}
		}
		catch (const std::exception& excep)
		{
			ret->Error = Converter::Convert(excep.what());
		}
		return ret;
	}));
}

ResultPtr MultivariateTimeSeriesFeatureExtractionLayer::IsSupportConnectFrom(_type prev)
{
	bool ret = prev->IsBaseOf(_typeof(TimeSeriesLayer));
	return ret ? std::make_shared<Result>(true) : std::make_shared<Result>(false, L"prev layer must be TimeSeriesLayer");
}

ResultPtr MultivariateTimeSeriesFeatureExtractionLayer::IsSupportConnectTo(_type next)
{
	bool ret = next->IsBaseOf(_typeof(Layer));
	return ret ? std::make_shared<Result>(true) : std::make_shared<Result>(false, L"next layer is not a Layer");
}

ResultPtr MultivariateTimeSeriesFeatureExtractionLayer::ReadyCore()
{
	if (!usedata) return std::make_shared<Result>(true);
	return !inpacket.expired() ? std::make_shared<Result>(true) : std::make_shared<Result>(false, L"data in expired");
}

ResultPtr MultivariateTimeSeriesFeatureExtractionLayer::InCore(data::DataPacketPtr data, data::ParameterPacketPtr prevparameter, _type prev)
{
	if (!data->GetType()->IsBaseOf(_typeof(TimeSeriesDataPacket))) return std::make_shared<Result>(false, L"prev data packet must be TimeSeriesDataPacket");
	
	if (usedata)
	{
		bool hascollection = false;
		if (data->HasSection(L"normalized_collection")) hascollection = true;
		if (data->HasSection(L"pairwise_collection")) hascollection = true;		
		if(!hascollection) return std::make_shared<Result>(false, L"prev data packet has no collection");
		inpacket = std::dynamic_pointer_cast<TimeSeriesDataPacket>(data);
	}
	return std::make_shared<Result>(true);
}

ResultPtr MultivariateTimeSeriesFeatureExtractionLayer::OutCore(data::DataPacketPtr& data, data::ParameterPacketPtr& parameter, _type next)
{
	data = outpacket;
	parameter = params;
	return std::make_shared<Result>(true);
}

ResultPtr MultivariateTimeSeriesFeatureExtractionLayer::ConfigCore(data::ParameterPacketPtr param)
{
	params = param;
	try
	{
		usedata = params->GetItem<bool>(L"usedata");
	}
	catch (...)
	{
		usedata = true;
	}
	return std::make_shared<Result>(true);
}

ResultPtr MultivariateTimeSeriesFeatureExtractionLayer::AddOperator(OperatorPtr op)
{
	if (!op) return std::make_shared<Result>(false, L"operator is null");
	MultivariateTimeSeriesFeatureExtractionLayerOperatorPtr op_ptr = std::dynamic_pointer_cast<MultivariateTimeSeriesFeatureExtractionLayerOperator>(op);
	if (!op_ptr) return std::make_shared<Result>(false, L"operator must be a MultivariateTimeSeriesFeatureExtractionLayerOperator");
	operators.push_back(op_ptr);
	return std::make_shared<Result>(true);
}

ResultPtr MultivariateTimeSeriesFeatureExtractionLayer::ConfigBatch(unsigned int batch)
{
	batchmode = true;
	curbatch = batch;
	return std::make_shared<Result>(true);
}

MultivariateTimeSeriesFeatureExtractionLayerOperator::MultivariateTimeSeriesFeatureExtractionLayerOperator()
{

}

MultivariateTimeSeriesFeatureExtractionLayerOperator::~MultivariateTimeSeriesFeatureExtractionLayerOperator()
{

}

ResultPtr MultivariateTimeSeriesFeatureExtractionLayerOperator::Run(TimeSeriesDataPacketPtr indata, data::ParameterPacketPtr inparam, _type intype, data::ParameterPacketPtr param, MultivariateTimeSeriesFeaturePacketPtr outdata, std::atomic<bool>& cancel)
{
	return std::make_shared<Result>(false);
}

ResultPtr MultivariateTimeSeriesFeatureExtractionLayerOperator::RunBatch(int batch, TimeSeriesDataPacketPtr indata, data::ParameterPacketPtr inparam, _type intype, data::ParameterPacketPtr param, MultivariateTimeSeriesFeaturePacketPtr outdata, std::atomic<bool>& cancel)
{
	return std::make_shared<Result>(false);
}
