#include "PeriodicClusteringOperator.h"
#include "MultivariateTimeSeriesFeatureExtractionLayer.h"
#include "MultivariateTimeSeriesFeaturePacket.h"
#include "Clustering.h"
#include <concurrent_unordered_map.h>
#include "..\Graphics\Graphics.h"

using namespace rsdn;
using namespace rsdn::learning;
using namespace rsdn::learning::timeseries;
using namespace rsdn::learning::timeseries::operators;

ResultPtr PeriodicClusteringOperator::RunChannelCPU(TimeSeriesDataPacketPtr indata, data::ParameterPacketPtr inparam, int channel, bool uselabels, std::shared_ptr<GenericBuffer<int>> reflabels, int label_distfun, BufferPtr timefeaturesbuf, BufferPtr timesbuf, std::shared_ptr<GenericBuffer<int>> labelsbuf, MultivariateTimeSeriesFeaturePacketPtr outdata, std::atomic<bool>& cancel)
{
	ResultPtr ret = std::make_shared<Result>(false);
	try
	{
		int featnum = timefeaturesbuf->Shape()[1];
		int num = timefeaturesbuf->Shape()[0];
		datatype* timefeatures = timefeaturesbuf->GetCpu();
		datatype* times = timesbuf->GetCpu();
		int* labels = labelsbuf->GetCpu();

		if (label_distfun < 1 || label_distfun>1)
		{
			ret->Message = L"invalid parameter: label_distancefunction";
			throw 1;
		}
		if (label_distfun == 1)
		{
			float lamda = 10.0f;
			try
			{
				lamda = params->GetItem<float>(L"lamda");
			}
			catch (...)
			{
				ret->Message = L"invalid parameter : lamda";
				throw 2;
			}

			float labeldistancebaseterm = 2.0f;
			try
			{
				labeldistancebaseterm = params->GetItem<float>(L"labeldistancebaseterm");
			}
			catch (...)
			{
				ret->Message = L"invalid parameter : labeldistancebaseterm";
				throw 2;
			}

			float continuousedge = 0.1f;
			try
			{
				continuousedge = params->GetItem<float>(L"continuousedge");
			}
			catch (...)
			{
				ret->Message = L"invalid parameter : continuousedge";
				throw 3;
			}

			int sparse = -1;
			try
			{
				sparse = params->GetItem<int>(L"sparse");
			}
			catch (...)
			{
			}

			TimeSeriesChannelSectionPtr tscsptr = std::make_shared<TimeSeriesChannelSection>();
			DTWCluster::ClusteringLP(timefeatures, labels, featnum, num, lamda, reflabels->GetCpu(), reflabels->Shape()[0], labeldistancebaseterm, sparse, tscsptr->Features, tscsptr->Labels);

			if (continuousedge > 1.f) continuousedge = 1.f;
			if (continuousedge < 0.f) continuousedge = 0.f;
			int edgenum = (int)(continuousedge * featnum);

			int templatenum = tscsptr->Features->Shape()[1];
			for (int ti = 0; ti < templatenum; ti++)
			{
				auto tidata = tscsptr->Features->GetCpu() + ti * featnum;
				datatype connectmidval = (tidata[0] + tidata[featnum - 1]) * 0.5f;

				datatype lplus = connectmidval - tidata[featnum - 1];
				datatype rmius = tidata[0] - connectmidval;

				int lt = featnum - 1 - edgenum;
				for (int i = featnum - edgenum; i < featnum; i++)
				{
					tidata[i] += lplus* (datatype)(i - lt) / edgenum;
				}

				for (int i = 0; i < edgenum; i++)
				{
					tidata[i] -= rmius* (datatype)(edgenum - i) / edgenum;
				}
			}

			auto templatechs = outdata->GetTemporary<concurrency::concurrent_unordered_map<int, TimeSeriesChannelSectionPtr>>(L"omp_template_channels");
			templatechs->insert(std::make_pair(channel, tscsptr));
			ret->State = true;
		}
		
	}
	catch (...)
	{

	}
	return ret;
}

ResultPtr PeriodicClusteringOperator::RunCPU(TimeSeriesDataPacketPtr indata, data::ParameterPacketPtr inparam, bool uselabels, std::shared_ptr<GenericBuffer<int>> reflabels, int label_distfun, MultivariateTimeSeriesFeaturePacketPtr outdata, std::atomic<bool>& cancel)
{
	ResultPtr ret = std::make_shared<Result>(false);
	try
	{
		auto normcolsec = indata->GetSection(L"normalized_collection");
		auto chnum = normcolsec->GetChildCount();

		outdata->SetTemporary(L"omp_template_channels", std::make_shared<concurrency::concurrent_unordered_map<int, TimeSeriesChannelSectionPtr>>());

		#pragma omp parallel for num_threads(Runtime::CpuJobCount())  
		for (__int64 i = 0; i < chnum; i++)
		{
			auto chsec = normcolsec->GetChild(i);
			if (!chsec) continue;
			BufferPtr timefeaturesbuf;
			try
			{
				timefeaturesbuf = chsec->GetData<Buffer>(L"timefeatures");
			}
			catch (...)
			{
				ret->Message = L"timefeatures is invalid at index=" + std::to_wstring(i) + L" at normalized collection in PeriodicClusteringOperator";
				throw 1;
			}
			BufferPtr timesbuf;
			try
			{
				timesbuf = chsec->GetData<Buffer>(L"times");
			}
			catch (...)
			{
				ret->Message = L"times is invalid at index=" + std::to_wstring(i) + L" at normalized collection in PeriodicClusteringOperator";
				throw 2;
			}
			std::shared_ptr<GenericBuffer<int>> labelsbuf;
			if (uselabels)
			{
				try
				{
					labelsbuf = chsec->GetData<GenericBuffer<int>>(L"labels");
				}
				catch (...)
				{
					ret->Message = L"labels is invalid at index=" + std::to_wstring(i) + L" at normalized collection in PeriodicClusteringOperator";
					throw 3;
				}
			}

			auto subret = RunChannelCPU(indata, inparam, i, uselabels, reflabels, label_distfun, timefeaturesbuf, timesbuf, labelsbuf, outdata, cancel);
			if (!subret->State)
			{
				ret = subret;
				throw 1;
			}
			Logger::Get().Report(rsdn::LogLevel::Info) << L"channel " << i << " is finished" <<Logger::endl;
		}

		auto templatechs = outdata->GetTemporary<concurrency::concurrent_unordered_map<int, TimeSeriesChannelSectionPtr>>(L"omp_template_channels");
		for (__int64 i = 0; i < chnum; i++)
		{
			outdata->TemplateSection->Channels->push_back(templatechs->at(i));
		}
		outdata->ClearTemporary(L"omp_template_channels");
		outdata->SetSection(L"templates", outdata->TemplateSection);

		ret->State = true;
	}
	catch (const std::exception& excep)
	{
		ret->Error = Converter::Convert(excep.what());
	}
	catch (...)
	{

	}
	return ret;
}

ResultPtr PeriodicClusteringOperator::ConfigCore(data::ParameterPacketPtr param)
{
	params = param;
	return std::make_shared<Result>(true);
}

ResultPtr PeriodicClusteringOperator::Run(TimeSeriesDataPacketPtr indata, data::ParameterPacketPtr inparam, _type intype, data::ParameterPacketPtr param, MultivariateTimeSeriesFeaturePacketPtr outdata, std::atomic<bool>& cancel)
{
	ResultPtr ret = std::make_shared<Result>(false);

	/*load joint registration*/
	bool uselabels;
	std::shared_ptr<GenericBuffer<int>> labels;
	int dist_fun;
	try
	{
		uselabels = params->GetItem<bool>(L"use_labels");
	}
	catch (const boost::bad_any_cast&)
	{
		ret->Message = L"invalid parameter: use_labels";
		return ret;
	}

	try
	{
		labels = std::dynamic_pointer_cast<GenericBuffer<int>>(params->GetItemEx(L"labels"));
	}
	catch (const boost::bad_any_cast&)
	{
		ret->Message = L"invalid parameter: labels";
		return ret;
	}

	try
	{
		dist_fun = params->GetItem<int>(L"label_distancefunction");
	}
	catch (const boost::bad_any_cast&)
	{
		ret->Message = L"invalid parameter: label_distancefunction";
		return ret;
	}

	if (Runtime::Mode() == ComputationMode::CPU)
	{
		ret = RunCPU(indata, inparam, uselabels, labels, dist_fun, outdata, cancel);
	}
	else
	{
		throw std::exception("gpu mode is not implemented");
	}

	return ret;
}

ResultPtr PeriodicClusteringOperator::RunBatch(int batch, TimeSeriesDataPacketPtr indata, data::ParameterPacketPtr inparam, _type intype, data::ParameterPacketPtr param, MultivariateTimeSeriesFeaturePacketPtr outdata, std::atomic<bool>& cancel)
{
	ResultPtr ret = std::make_shared<Result>(false);
	bool uselabels = true;
	std::shared_ptr<GenericBuffer<int>> labels;
	int dist_fun;
	try
	{
		uselabels = params->GetItem<bool>(L"use_labels");
	}
	catch (...)
	{
		ret->Message = L"invalid parameter: use_labels";
		return ret;
	}

	try
	{
		labels = std::dynamic_pointer_cast<GenericBuffer<int>>(params->GetItemEx(L"labels"));
	}
	catch (...)
	{
		ret->Message = L"invalid parameter: labels";
		return ret;
	}

	try
	{
		dist_fun = params->GetItem<int>(L"label_distancefunction");
	}
	catch (...)
	{
		ret->Message = L"invalid parameter: label_distancefunction";
		return ret;
	}

	if (Runtime::Mode() == ComputationMode::CPU)
	{
		ret = RunBatchCPU(batch, indata, inparam, uselabels, labels, dist_fun, outdata, cancel);
	}
	else
	{
		throw std::exception("gpu mode is not implemented");
	}

	return ret;
}

ResultPtr PeriodicClusteringOperator::RunChannelBatchCPU(int batch, TimeSeriesDataPacketPtr indata, data::ParameterPacketPtr inparam, int channel, bool uselabels, std::shared_ptr<GenericBuffer<int>> reflabels, int label_distfun, BufferPtr timefeaturesbuf, BufferPtr timesbuf, std::shared_ptr<GenericBuffer<int>> labelsbuf, MultivariateTimeSeriesFeaturePacketPtr outdata, std::atomic<bool>& cancel)
{
	ResultPtr ret = std::make_shared<Result>(false);
	try
	{
		int featnum = timefeaturesbuf->Shape()[1];
		int num = timefeaturesbuf->Shape()[0];
		datatype* timefeatures = timefeaturesbuf->GetCpu();
		int* labels = labelsbuf->GetCpu();

		if (label_distfun !=2 )
		{
			ret->Message = L"invalid parameter: label_distancefunction";
			throw 1;
		}
		if (label_distfun == 2)
		{
			float continuousedge = 0.1f;
			try
			{
				continuousedge = params->GetItem<float>(L"continuousedge");
			}
			catch (...)
			{
				ret->Message = L"invalid parameter : continuousedge";
				throw 3;
			}

			int sparse = -1;
			try
			{
				sparse = params->GetItem<int>(L"sparse");
			}
			catch (...)
			{
			}

			int dbscan_order = -1;
			try
			{
				dbscan_order = params->GetItem<int>(L"dbscan_order");
			}
			catch (...)
			{

			}

			TimeSeriesChannelSectionPtr tscsptr = std::make_shared<TimeSeriesChannelSection>();
			DTWCluster::ClusteringS(timefeatures, labels, featnum, num, sparse, tscsptr->Features, tscsptr->Labels, dbscan_order);

			if (continuousedge > 1.f) continuousedge = 1.f;
			if (continuousedge < 0.f) continuousedge = 0.f;
			int edgenum = (int)(continuousedge * featnum);

			int templatenum = tscsptr->Features->Shape()[0];
			for (int ti = 0; ti < templatenum; ti++)
			{
				auto tidata = tscsptr->Features->GetCpu() + ti * featnum;
				datatype connectmidval = (tidata[0] + tidata[featnum - 1]) * 0.5f;

				datatype lplus = connectmidval - tidata[featnum - 1];
				datatype rmius = tidata[0] - connectmidval;

				int lt = featnum - 1 - edgenum;
				for (int i = featnum - edgenum; i < featnum; i++)
				{
					tidata[i] += lplus* (datatype)(i - lt) / edgenum;
				}

				for (int i = 0; i < edgenum; i++)
				{
					tidata[i] -= rmius* (datatype)(edgenum - i) / edgenum;
				}
			}

			auto templatechs = outdata->GetTemporary<concurrency::concurrent_unordered_map<int, TimeSeriesChannelSectionPtr>>(L"omp_template_channels");
			templatechs->insert(std::make_pair(channel, tscsptr));
			ret->State = true;
		}

	}
	catch (...)
	{

	}
	return ret;
}

ResultPtr PeriodicClusteringOperator::RunBatchCPU(int batch, TimeSeriesDataPacketPtr indata, data::ParameterPacketPtr inparam, bool uselabels, std::shared_ptr<GenericBuffer<int>> reflabels, int label_distfun, MultivariateTimeSeriesFeaturePacketPtr outdata, std::atomic<bool>& cancel)
{
	ResultPtr ret = std::make_shared<Result>(false);
	try
	{
		auto normcolsec = indata->GetSection(L"normalized_collection");
		auto chnum = normcolsec->GetChildCount();

		outdata->SetTemporary(L"omp_template_channels", std::make_shared<concurrency::concurrent_unordered_map<int, TimeSeriesChannelSectionPtr>>());

		//#pragma omp parallel for num_threads(Runtime::CpuJobCount())  
		for (__int64 i = 0; i < chnum; i++)
		{
			auto chsec = normcolsec->GetChild(i);
			if (!chsec) continue;
			BufferPtr timefeaturesbuf;
			try
			{
				timefeaturesbuf = chsec->GetData<Buffer>(L"timefeatures");
			}
			catch (...)
			{
				ret->Message = L"timefeatures is invalid at index=" + std::to_wstring(i) + L" at normalized collection in PeriodicClusteringOperator";
				throw 1;
			}
			BufferPtr timesbuf;
			try
			{
				timesbuf = chsec->GetData<Buffer>(L"times");
			}
			catch (...)
			{
			}
			std::shared_ptr<GenericBuffer<int>> labelsbuf;
			if (uselabels)
			{
				try
				{
					labelsbuf = chsec->GetData<GenericBuffer<int>>(L"labels");
				}
				catch (...)
				{
					ret->Message = L"labels is invalid at index=" + std::to_wstring(i) + L" at normalized collection in PeriodicClusteringOperator";
					throw 3;
				}
			}

			auto subret = RunChannelBatchCPU(batch, indata, inparam, i, uselabels, reflabels, label_distfun, timefeaturesbuf, timesbuf, labelsbuf, outdata, cancel);
			if (!subret->State)
			{
				ret = subret;
				throw 1;
			}
			Logger::Get().Report(rsdn::LogLevel::Info) << L"batch[" << batch <<L"] clustering for channel " << i << " is finished" << Logger::endl;
		}

		auto templatechs = outdata->GetTemporary<concurrency::concurrent_unordered_map<int, TimeSeriesChannelSectionPtr>>(L"omp_template_channels");
		outdata->TemplateSection->Channels->resize(chnum);
		for (__int64 i = 0; i < chnum; i++)
		{
			outdata->TemplateSection->Channels->at(i) = templatechs->at(i);
		}
		outdata->ClearTemporary(L"omp_template_channels");
		outdata->SetTemporary(L"batch", std::make_shared<int>(batch));
		outdata->SetSection(L"templates", outdata->TemplateSection);

		ret->State = true;
	}
	catch (const std::exception& excep)
	{
		ret->Error = Converter::Convert(excep.what());
	}
	catch (...)
	{

	}
	return ret;
}
