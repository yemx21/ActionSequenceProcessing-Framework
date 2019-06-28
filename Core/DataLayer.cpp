#include "DataLayer.h"
using namespace rsdn;
using namespace rsdn::layer;


DataLayer::DataLayer()
{
	file = std::make_shared<FileStream>();
	bsdb = std::make_shared<data::BSDBReader>();
}

DataLayer::~DataLayer()
{
}

std::unique_ptr<std::future<ResultPtr>> DataLayer::RunAsync(std::atomic<bool>& cancel)
{
	return nullptr;
}

bool DataLayer::Open(const std::wstring& path)
{
	return false;
}

