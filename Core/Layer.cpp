#include "Layer.h"
using namespace rsdn;
using namespace rsdn::data;
using namespace rsdn::layer;


static boost::any ANY_EMPTY{};

Result::Result() : _state(false)
{

}

Result::Result(bool state): _state(state)
{

}

Result::Result(bool state, const std::wstring& msg): _state(state), _msg(msg)
{

}

Result::Result(bool state, const std::wstring& msg, const std::wstring& err) : _state(state), _msg(msg), _err(err)
{

}


bool Result::GetState() const
{
	return _state;

}
void Result::SetState(bool val)
{
	_state = val;
}

const std::wstring& Result::GetMsg() const
{
	return _msg;
}
void Result::SetMsg(const std::wstring& msg)
{
	_msg = msg;
}

const std::wstring& Result::GetError() const
{
	return _err;
}
void Result::SetError(const std::wstring& msg)
{
	_err = msg;
}

Result::operator bool() const
{
	return _state;
}

bool DataPacket::HasSection(const std::wstring& key)
{
	auto iter = sections.find(key);
	return iter != sections.end();
}

IDataSectionPtr DataPacket::GetSection(const std::wstring& key)
{
	auto iter = sections.find(key);
	if (iter != sections.end())
	{
		return iter->second;
	}
	return nullptr;
}

void DataPacket::SetSection(const std::wstring& key, IDataSectionPtr sec)
{
	auto iter = sections.find(key);
	if (iter != sections.end())
	{
		iter->second = sec;
	}
	else
	{
		sections.insert(std::make_pair(key, sec));
	}
}

void DataPacket::SetTemporaryCore(const std::wstring& key, std::shared_ptr<void> val)
{
	auto iter = temporary.find(key);
	if (iter != temporary.end())
	{
		iter->second = val;
	}
	else
	{
		temporary.insert(std::make_pair(key, val));
	}
}

void DataPacket::ClearTemporary(const std::wstring& key)
{
	temporary.erase(key);
}

std::shared_ptr<void> DataPacket::GetTemporaryCore(const std::wstring& key)
{
	auto iter = temporary.find(key);
	if (iter != temporary.end()) return iter->second;
	return nullptr;
}

boost::any IDataSection::GetDataCore(const std::wstring& key)
{
	return ANY_EMPTY;
}

void* IDataSection::GetDataCore1(const std::wstring& key)
{
	return nullptr;
}

void IDataSection::SetDataCore(const std::wstring& key, const boost::any& data)
{

}

bool IDataSection::HasSection(const std::wstring& key)
{
	return false;
}

IDataSectionPtr IDataSection::GetSection(const std::wstring& key)
{
	return nullptr;
}

void IDataSection::SetSection(const std::wstring& key, IDataSectionPtr sec)
{

}

__int64 IDataSection::GetChildCount()
{
	return 0ll;
}

IDataSectionPtr IDataSection::GetChild(const __int64 index)
{
	return nullptr;
}

void IDataSection::SetChild(const __int64 index, IDataSectionPtr sec)
{

}

int ParameterPacket::GetCountCore() const
{
	return 0;
}

int ParameterPacket::GetCount() const
{
	return GetCountCore();
}

ParameterItemType ParameterPacket::GetTypeCore(const std::wstring& key) const
{
	return ParameterItemType::None;
}

ParameterItemType ParameterPacket::GetTypeCore(int index) const
{
	return ParameterItemType::None;
}

IBufferPtr ParameterPacket::GetCoreEx(const std::wstring& key) const
{
	return nullptr;
}

IBufferPtr ParameterPacket::GetAtIndexCoreEx(int index) const
{
	return nullptr;
}

ParameterItemType ParameterPacket::GetItemType(const std::wstring& key) const
{
	return GetTypeCore(key);
}

ParameterItemType ParameterPacket::GetItemType(int index) const
{
	return GetTypeCore(index);
}

IBufferPtr ParameterPacket::GetItemEx(const std::wstring& key) const
{
	return GetCoreEx(key);
}

IBufferPtr ParameterPacket::GetItemAtIndexEx(int index) const
{
	return GetAtIndexCoreEx(index);
}

const boost::any& ParameterPacket::GetCore(const std::wstring& key) const
{
	return ANY_EMPTY;
}

const boost::any& ParameterPacket::GetAtIndexCore(int index) const
{
	return ANY_EMPTY;
}

const boost::any& ParameterPacket::GetItem(const std::wstring& key) const
{
	return GetCore(key);
}

const boost::any& ParameterPacket::GetItemAtIndex(int index) const
{
	return GetAtIndexCore(index);
}

ResultPtr Operator::ConfigCore(data::ParameterPacketPtr param)
{
	return std::make_shared<Result>(false);
}

ResultPtr Operator::Config(data::ParameterPacketPtr param)
{
	return ConfigCore(param);
}

bool Operator::IsExclusive() const
{
	return false;
}

Layer::Layer() 
{
	_id = -1;
}

int64_t Layer::GetId() const
{
	return _id;
}

ResultPtr Layer::IsSupportConnectFrom(_type)
{
	return std::make_shared<Result>(false);
}

ResultPtr Layer::IsSupportConnectTo(_type)
{
	return std::make_shared<Result>(false);
}

ResultPtr Layer::ConfigCore(data::ParameterPacketPtr parameter)
{
	return std::make_shared<Result>(true);
}

ResultPtr Layer::ReadyCore()
{
	return std::make_shared<Result>(false);
}

ResultPtr Layer::InCore(data::DataPacketPtr data, data::ParameterPacketPtr prevparameter, _type prev)
{
	return std::make_shared<Result>(false);
}

ResultPtr Layer::OutCore(data::DataPacketPtr& data, data::ParameterPacketPtr& outparameter, _type next)
{
	return std::make_shared<Result>(false);
}

ResultPtr Layer::Ready()
{
	return ReadyCore();
}

ResultPtr Layer::In(data::DataPacketPtr data, data::ParameterPacketPtr prevparameter, _type prev)
{
	if (!data) return std::make_shared<Result>(false, L"no data");
	if (!IsSupportConnectFrom(prev)) return std::make_shared<Result>(false, L"unsupported input layer");
	return InCore(data, prevparameter, prev);
}

ResultPtr Layer::Out(data::DataPacketPtr& data, data::ParameterPacketPtr& outparameter, _type next)
{
	if (!IsSupportConnectTo(next)) return std::make_shared<Result>(false, L"unsupported output layer");
	return OutCore(data, outparameter, next);
}

std::unique_ptr<std::future<ResultPtr>> Layer::RunAsync(std::atomic<bool>& cancel)
{
	return nullptr;
}

ResultPtr Layer::Config(data::ParameterPacketPtr param)
{
	return ConfigCore(param);
}

ResultPtr Layer::AddOperator(OperatorPtr op)
{
	return std::make_shared<Result>(false);
}

unsigned int Layer::GetBatchCount()
{
	return 0u;
}

std::unique_ptr<std::future<ResultPtr>> Layer::RunBatchAsync(std::atomic<bool>& cancel, unsigned int batch)
{
	return nullptr;
}

ResultPtr Layer::ConfigBatch(unsigned int batch)
{
	return std::make_shared<Result>(false);
}

std::unique_ptr<std::future<ResultPtr>> Layer::PrepareBatchAsync(std::atomic<bool>& cancel)
{
	return nullptr;
}

AsyncResult::AsyncResult()
{

}

AsyncResult::~AsyncResult()
{
	
}

void AsyncResult::OnCancel()
{
	
}

void AsyncResult::OnWait()
{
	
}

ResultPtr AsyncResult::OnGet()
{
	return std::make_shared<Result>(false, L"no implementation");
}


void AsyncResult::Cancel()
{
	OnCancel();
}

void AsyncResult::Wait()
{
	OnWait();
}

ResultPtr AsyncResult::Get()
{
	return OnGet();
}