#include "PythonParameterPacket.h"
#include <boost/algorithm/string.hpp>
#include <istream>
#include <boost/locale.hpp>
#include <filesystem>
using namespace rsdn;
using namespace rsdn::data;

#pragma region PythonParameterPacket

int PythonParameterPacket::GetCountCore() const
{
	return (int)indices.size();
}

const boost::any& PythonParameterPacket::GetCore(const std::wstring& key) const
{
	auto params_found = params.find(key);
	if (params_found != params.end())
	{
		return params_found->second;
	}
	return ParameterPacket::GetCore(key);
}

const boost::any& PythonParameterPacket::GetAtIndexCore(int index) const
{
	auto indices_found = indices.find(index);
	if (indices_found != indices.end())
	{
		auto params_found = params.find(indices_found->second);
		if (params_found != params.end())
		{
			return params_found->second;
		}
	}
	return ParameterPacket::GetAtIndexCore(index);
}

IBufferPtr PythonParameterPacket::GetCoreEx(const std::wstring& key) const
{
	auto buffers_found = buffers.find(key);
	if (buffers_found != buffers.end())
	{
		return buffers_found->second;
	}
	return nullptr;
}

IBufferPtr PythonParameterPacket::GetAtIndexCoreEx(int index) const
{
	auto indices_found = indices.find(index);
	if (indices_found != indices.end())
	{
		auto buffers_found = buffers.find(indices_found->second);
		if (buffers_found != buffers.end())
		{
			return buffers_found->second;
		}
	}
	return nullptr;
}

ParameterItemType PythonParameterPacket::GetTypeCore(const std::wstring& key) const
{
	auto params_found = params.find(key);
	if (params_found != params.end())
	{
		return ParameterItemType::Value;
	}
	else
	{
		auto buffers_found = buffers.find(key);
		if (buffers_found != buffers.end())
		{
			return ParameterItemType::Buffer;
		}
	}
	return ParameterItemType::None;
}

ParameterItemType PythonParameterPacket::GetTypeCore(int index) const
{
	auto indices_found = indices.find(index);
	if (indices_found != indices.end())
	{
		auto params_found = params.find(indices_found->second);
		if (params_found != params.end())
		{
			return ParameterItemType::Value;
		}
		else
		{
			auto buffers_found = buffers.find(indices_found->second);
			if (buffers_found != buffers.end())
			{
				return ParameterItemType::Buffer;
			}
		}
	}
	return ParameterItemType::None;
}

#pragma endregion
