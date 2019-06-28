#include "ParameterLoader.h"
#include "StringDerivedParameterPacket.h"
#include <boost/algorithm/string.hpp>
#include <istream>
#include <boost/locale.hpp>
#include <filesystem>
using namespace rsdn;
using namespace rsdn::data;

#pragma region StringDerivedParameterPacket

int StringDerivedParameterPacket::GetCountCore() const
{
	return (int)indices.size();
}

const boost::any& StringDerivedParameterPacket::GetCore(const std::wstring& key) const
{
	auto params_found = params.find(key);
	if (params_found != params.end())
	{
		return params_found->second;
	}
	return ParameterPacket::GetCore(key);
}

const boost::any& StringDerivedParameterPacket::GetAtIndexCore(int index) const
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

IBufferPtr StringDerivedParameterPacket::GetCoreEx(const std::wstring& key) const
{
	auto buffers_found = buffers.find(key);
	if (buffers_found != buffers.end())
	{
		return buffers_found->second;
	}
	return nullptr;
}

IBufferPtr StringDerivedParameterPacket::GetAtIndexCoreEx(int index) const
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

ParameterItemType StringDerivedParameterPacket::GetTypeCore(const std::wstring& key) const
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

ParameterItemType StringDerivedParameterPacket::GetTypeCore(int index) const
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

ParameterBaseType StringDerivedParameterPacket::FindBaseType(const std::wstring& name)
{
	if (name.empty()) return ParameterBaseType::None;
	if (name.compare(L"bool")==0 || name.compare(L"int1")==0)
	{
		return ParameterBaseType::Boolean;
	}
	else if (name.compare(L"char") == 0 || name.compare(L"int8") == 0)
	{
		return ParameterBaseType::Int8;
	}
	else if (name.compare(L"uchar") == 0 || name.compare(L"uint8") == 0)
	{
		return ParameterBaseType::UInt8;
	}
	else if (name.compare(L"int16") == 0 || name.compare(L"short") == 0)
	{
		return ParameterBaseType::Int16;
	}
	else if (name.compare(L"uint16") == 0 || name.compare(L"ushort") == 0)
	{
		return ParameterBaseType::UInt16;
	}
	else if (name.compare(L"int32") == 0 || name.compare(L"int") == 0)
	{
		return ParameterBaseType::Int32;
	}
	else if (name.compare(L"uint32") == 0 || name.compare(L"uint") == 0)
	{
		return ParameterBaseType::UInt32;
	}
	else if (name.compare(L"int64") == 0)
	{
		return ParameterBaseType::Int64;
	}
	else if (name.compare(L"uint64") == 0)
	{
		return ParameterBaseType::UInt64;
	}
	else if (name.compare(L"float") == 0 || name.compare(L"float32") == 0 || name.compare(L"single") == 0)
	{
		return ParameterBaseType::Float;
	}
	else if (name.compare(L"double") == 0 || name.compare(L"float64") == 0)
	{
		return ParameterBaseType::Double;
	}
	else if (name.compare(L"string") == 0 || name.compare(L"str") == 0)
	{
		return ParameterBaseType::String;
	}
	return ParameterBaseType::Unknown;
}

ParameterBufferType StringDerivedParameterPacket::FindBufferType(const std::wstring& name)
{
	if (name.empty()) return ParameterBufferType::None;
	if (name.compare(L"buffer") == 0 || name.compare(L"base") == 0)
	{
		return ParameterBufferType::Buffer;
	}
	else if (name.compare(L"genericbuffer") == 0 || name.compare(L"generic") == 0)
	{
		return ParameterBufferType::GenericBuffer;
	}
	else if (name.compare(L"binarybuffer") == 0 || name.compare(L"binary") == 0)
	{
		return ParameterBufferType::BinaryBuffer;
	}
	return ParameterBufferType::Unknown;
}

bool StringDerivedParameterPacket::TryParse(ParameterBaseType base, const std::wstring& eval, boost::any& obj)
{
	bool ret = false;
	try
	{
		switch (base)
		{
			case ParameterBaseType::Boolean:
			{
				if (eval.compare(L"true") == 0 || eval.compare(L"True") == 0)
				{
					obj = true;
					ret = true;
				}
				else if (eval.compare(L"false") == 0 || eval.compare(L"False") == 0)
				{
					obj = false;
					ret = true;
				}
				else
				{
					auto num = std::stod(eval);
					obj = num == 0 ? false : true;
					ret = true;
				}
			}
			break;
			case ParameterBaseType::Int8:
			{					
				auto num = std::stoi(eval);
				obj = (int8_t)num;
				ret = true;
			}
			break;
			case ParameterBaseType::UInt8:
			{
				auto num = std::stoi(eval);
				obj = (uint8_t)num;
				ret = true;
			}
			break;
			case ParameterBaseType::Int16:
			{
				auto num = std::stoi(eval);
				obj = (int16_t)num;
				ret = true;
			}
			break;
			case ParameterBaseType::UInt16:
			{
				auto num = std::stoi(eval);
				obj = (uint16_t)num;
				ret = true;
			}
			break;
			case ParameterBaseType::Int32:
			{
				auto num = std::stoi(eval);
				obj = (int32_t)num;
				ret = true;
			}
			break;
			case ParameterBaseType::UInt32:
			{
				auto num = std::stol(eval);
				obj = (uint32_t)num;
				ret = true;
			}
			break;
			case ParameterBaseType::Int64:
			{
				auto num = std::stoll(eval);
				obj = (int32_t)num;
			}
			break;
			case ParameterBaseType::UInt64:
			{
				auto num = std::stoull(eval);
				obj = (uint64_t)num;
				ret = true;
			}
			break;
			case ParameterBaseType::Float:
			{
				auto num = std::stof(eval);
				obj = (float)num;
				ret = true;
			}
			break;
			case ParameterBaseType::Double:
			{
				auto num = std::stod(eval);
				obj = (uint64_t)num;
				ret = true;
			}
			break;
			case ParameterBaseType::String:
			{
				obj = std::wstring(eval);
				ret = true;
			}
			break;
		}		
	}
	catch (...)
	{

	}
	return ret;
}

bool StringDerivedParameterPacket::TryParseBuffer(ParameterBaseType base, ParameterBufferType bufty, const std::wstring& lenexpr, const std::wstring& eval, IBufferPtr& obj)
{
	try
	{
		boost::locale::generator gen;
		std::locale sysloc = gen("");

		if (bufty == ParameterBufferType::Buffer)
		{
			size_t lenspos1 = eval.find(L'<');
			if (lenspos1 != std::wstring::npos)
			{
				size_t lenepos2 = eval.find(L'>', lenspos1 + 1);
				if (lenepos2 != std::wstring::npos)
				{
					BufferPtr buf = std::make_shared<Buffer>();
					if (buf->Load(eval.substr(lenspos1 + 1, lenepos2 - lenspos1 - 1)))
					{
						obj = buf;
					}
				}
			}
		}
		else if(bufty == ParameterBufferType::GenericBuffer)
		{
			size_t lenspos1 = eval.find(L'<');
			if (lenspos1 != std::wstring::npos)
			{
				size_t lenepos2 = eval.find(L'>', lenspos1 + 1);
				if (lenepos2 != std::wstring::npos)
				{
					BufferPtr buf = std::make_shared<Buffer>();
					if (buf->Load(eval.substr(lenspos1 + 1, lenepos2 - lenspos1 - 1)))
					{
						obj = buf;
					}
				}
			}
			else
			{
				if(!lenexpr.empty())
				{
					boost::locale::generator gen;
					std::locale sysloc = gen("");
					std::vector<std::wstring> lenstrs;
					boost::split(lenstrs, lenexpr, boost::is_any_of(L","));
					std::vector<int> shape;
					for (auto& str : lenstrs)
					{
						boost::trim(str, sysloc);
						shape.push_back(std::stoi(str));
					}
					size_t pos = 0;
					switch (base)
					{
					case ParameterBaseType::Boolean:
					{
						std::shared_ptr<GenericBuffer<bool>> buf = std::make_shared<GenericBuffer<bool>>(shape);
						auto cpubuf = buf->GetCpu();						
						size_t counter = 0;
						while (pos != std::wstring::npos)
						{
							auto nxtpos = eval.find_first_of(L',', pos);
							if (nxtpos != std::wstring::npos)
							{
								std::wstring ceval = eval.substr(pos, nxtpos - pos);
								if (ceval.compare(L"true"))
								{
									cpubuf[counter++] = true;
								}
								else if (ceval.compare(L"false"))
								{
									cpubuf[counter++] = false;
								}
								else
								{
									auto num = std::stod(ceval);
									cpubuf[counter++] = num == 0 ? false : true;
								}
								pos = nxtpos + 1;
							}
							else
							{
								std::wstring ceval = eval.substr(pos);
								if (ceval.compare(L"true"))
								{
									cpubuf[counter++] = true;
								}
								else if (ceval.compare(L"false"))
								{
									cpubuf[counter++] = false;
								}
								else
								{
									auto num = std::stod(ceval);
									cpubuf[counter++] = num == 0 ? false : true;
								}
							}
						};
						obj = buf;
					}
					break;
					case ParameterBaseType::Int8:
					{
						std::shared_ptr<GenericBuffer<int8_t>> buf = std::make_shared<GenericBuffer<int8_t>>(shape);
						auto cpubuf = buf->GetCpu();
						size_t counter = 0;
						while (pos != std::wstring::npos)
						{
							auto nxtpos = eval.find_first_of(L',', pos);
							if (nxtpos != std::wstring::npos)
							{
								std::wstring ceval = eval.substr(pos, nxtpos - pos);
								cpubuf[counter++] = std::stoi(ceval);
								pos = nxtpos + 1;
							}
							else
							{
								cpubuf[counter++] = std::stoi(eval.substr(pos));
								break;
							}
						};
						obj = buf;
					}
					break;
					case ParameterBaseType::UInt8:
					{
						std::shared_ptr<GenericBuffer<uint8_t>> buf = std::make_shared<GenericBuffer<uint8_t>>(shape);
						auto cpubuf = buf->GetCpu();
						size_t counter = 0;
						while (pos != std::wstring::npos)
						{
							auto nxtpos = eval.find_first_of(L',', pos);
							if (nxtpos != std::wstring::npos)
							{
								std::wstring ceval = eval.substr(pos, nxtpos - pos);
								cpubuf[counter++] = std::stoi(ceval);
								pos = nxtpos + 1;
							}
							else
							{
								cpubuf[counter++] = std::stoi(eval.substr(pos));
								break;
							}
						};
						obj = buf;
					}
					break;
					case ParameterBaseType::Int16:
					{
						std::shared_ptr<GenericBuffer<int16_t>> buf = std::make_shared<GenericBuffer<int16_t>>(shape);
						auto cpubuf = buf->GetCpu();
						size_t counter = 0;
						while (pos != std::wstring::npos)
						{
							auto nxtpos = eval.find_first_of(L',', pos);
							if (nxtpos != std::wstring::npos)
							{
								std::wstring ceval = eval.substr(pos, nxtpos - pos);
								cpubuf[counter++] = std::stoi(ceval);
								pos = nxtpos + 1;
							}
							else
							{
								cpubuf[counter++] = std::stoi(eval.substr(pos));
								break;
							}
						};
						obj = buf;
					}
					break;
					case ParameterBaseType::UInt16:
					{
						std::shared_ptr<GenericBuffer<uint16_t>> buf = std::make_shared<GenericBuffer<uint16_t>>(shape);
						auto cpubuf = buf->GetCpu();
						size_t counter = 0;
						while (pos != std::wstring::npos)
						{
							auto nxtpos = eval.find_first_of(L',', pos);
							if (nxtpos != std::wstring::npos)
							{
								std::wstring ceval = eval.substr(pos, nxtpos - pos);
								cpubuf[counter++] = std::stoi(ceval);
								pos = nxtpos + 1;
							}
							else
							{
								cpubuf[counter++] = std::stoi(eval.substr(pos));
								break;
							}
						};
						obj = buf;
					}
					break;
					case ParameterBaseType::Int32:
					{
						std::shared_ptr<GenericBuffer<int32_t>> buf = std::make_shared<GenericBuffer<int32_t>>(shape);
						auto cpubuf = buf->GetCpu();
						size_t counter = 0;
						while (pos != std::wstring::npos)
						{
							auto nxtpos = eval.find_first_of(L',', pos);
							if (nxtpos != std::wstring::npos)
							{
								std::wstring ceval = eval.substr(pos, nxtpos - pos);
								cpubuf[counter++] = std::stoi(ceval);
								pos = nxtpos + 1;
							}
							else
							{
								cpubuf[counter++] = std::stoi(eval.substr(pos));
								break;
							}
						};
						obj = buf;
					}
					break;
					case ParameterBaseType::UInt32:
					{
						std::shared_ptr<GenericBuffer<uint32_t>> buf = std::make_shared<GenericBuffer<uint32_t>>(shape);
						auto cpubuf = buf->GetCpu();
						size_t counter = 0;
						while (pos != std::wstring::npos)
						{
							auto nxtpos = eval.find_first_of(L',', pos);
							if (nxtpos != std::wstring::npos)
							{
								std::wstring ceval = eval.substr(pos, nxtpos - pos);
								cpubuf[counter++] = std::stoi(ceval);
								pos = nxtpos + 1;
							}
							else
							{
								cpubuf[counter++] = std::stoi(eval.substr(pos));
								break;
							}
						};
						obj = buf;
					}
					break;
					case ParameterBaseType::Int64:
					{
						std::shared_ptr<GenericBuffer<int64_t>> buf = std::make_shared<GenericBuffer<int64_t>>(shape);
						auto cpubuf = buf->GetCpu();
						size_t counter = 0;
						while (pos != std::wstring::npos)
						{
							auto nxtpos = eval.find_first_of(L',', pos);
							if (nxtpos != std::wstring::npos)
							{
								std::wstring ceval = eval.substr(pos, nxtpos - pos);
								cpubuf[counter++] = std::stoll(ceval);
								pos = nxtpos + 1;
							}
							else
							{
								cpubuf[counter++] = std::stoi(eval.substr(pos));
								break;
							}
						};
						obj = buf;
					}
					break;
					case ParameterBaseType::UInt64:
					{
						std::shared_ptr<GenericBuffer<uint64_t>> buf = std::make_shared<GenericBuffer<uint64_t>>(shape);
						auto cpubuf = buf->GetCpu();
						size_t counter = 0;
						while (pos != std::wstring::npos)
						{
							auto nxtpos = eval.find_first_of(L',', pos);
							if (nxtpos != std::wstring::npos)
							{
								std::wstring ceval = eval.substr(pos, nxtpos - pos);
								cpubuf[counter++] = std::stoull(ceval);
								pos = nxtpos + 1;
							}
							else
							{
								cpubuf[counter++] = std::stoi(eval.substr(pos));
								break;
							}
						};
						obj = buf;
					}
					break;
					case ParameterBaseType::Float:
					{
						std::shared_ptr<GenericBuffer<float>> buf = std::make_shared<GenericBuffer<float>>(shape);
						auto cpubuf = buf->GetCpu();
						size_t counter = 0;
						while (pos != std::wstring::npos)
						{
							auto nxtpos = eval.find_first_of(L',', pos);
							if (nxtpos != std::wstring::npos)
							{
								std::wstring ceval = eval.substr(pos, nxtpos - pos);
								cpubuf[counter++] = std::stof(ceval);
								pos = nxtpos + 1;
							}
							else
							{
								cpubuf[counter++] = std::stoi(eval.substr(pos));
								break;
							}
						};
						obj = buf;
					}
					break;
					case ParameterBaseType::Double:
					{
						std::shared_ptr<GenericBuffer<double>> buf = std::make_shared<GenericBuffer<double>>(shape);
						auto cpubuf = buf->GetCpu();
						size_t counter = 0;
						while (pos != std::wstring::npos)
						{
							auto nxtpos = eval.find_first_of(L',', pos);
							if (nxtpos != std::wstring::npos)
							{
								std::wstring ceval = eval.substr(pos, nxtpos - pos);
								cpubuf[counter++] = std::stod(ceval);
								pos = nxtpos + 1;
							}
							else
							{
								cpubuf[counter++] = std::stoi(eval.substr(pos));
								break;
							}
						};
						obj = buf;
					}
					break;
					case ParameterBaseType::String:
					{
						std::shared_ptr<GenericBuffer<std::wstring>> buf = std::make_shared<GenericBuffer<std::wstring>>(shape);
						auto cpubuf = buf->GetCpu();
						size_t counter = 0;
						while (pos != std::wstring::npos)
						{
							auto nxtpos = eval.find_first_of(L',', pos);
							if (nxtpos != std::wstring::npos)
							{
								std::wstring ceval = eval.substr(pos, nxtpos - pos);
								cpubuf[counter++] = ceval;
								pos = nxtpos + 1;
							}
							else
							{
								cpubuf[counter++] = std::stoi(eval.substr(pos));
								break;
							}
						};
						obj = buf;
					}
					break;
					}
				}
			}
		}
		else if (bufty == ParameterBufferType::BinaryBuffer)
		{
			size_t lenspos1 = eval.find(L'<');
			if (lenspos1 != std::wstring::npos)
			{
				size_t lenepos2 = eval.find(L'>', lenspos1 + 1);
				if (lenepos2 != std::wstring::npos)
				{
					BinaryBufferPtr buf = std::make_shared<BinaryBuffer>();
					if (buf->Load(eval.substr(lenspos1 + 1, lenepos2 - lenspos1 - 1)))
					{
						obj = buf;
					}
				}
			}
		}
	}
	catch(...)
	{
		obj = nullptr;
	}
	return obj != nullptr;
}

bool StringDerivedParameterPacket::TryAddByString(const std::wstring& expression, std::wstring& err)
{
	boost::locale::generator gen;
	std::locale sysloc = gen("");
	std::vector<std::wstring> strs;
	boost::split(strs, expression, boost::is_any_of(L"="));
	for (auto& str : strs) boost::trim(str, sysloc);
	if (strs.size() != 2)
	{
		err = L"invalid expression: '='";
		return false;
	}
	
	std::wstring name = strs[0];
	const std::wstring& valexpr = strs[1];

	if (valexpr[0] != L'[')
	{
		err = L"invalid value expression: '['";
		return false;
	}
	size_t basebrace = valexpr.find_first_of(L']');
	if (basebrace == valexpr.npos)
	{
		err = L"invalid value expression: ']'";
		return false;
	}
	std::wstring typedesc = valexpr.substr(1, basebrace - 1);	
	if (basebrace > valexpr.size() - 2)
	{
		err = L"invalid value expression: empty value";
		return false;
	}

	std::vector<std::wstring> descs;
	boost::split(descs, typedesc, boost::is_any_of(L";"));

	ParameterBaseType basetype = ParameterBaseType::None;
	ParameterBufferType buftype = ParameterBufferType::None;
	std::wstring sizedesc;
	size_t val_startpos = basebrace + 1;

	if (descs.size() == 1)
	{
		basetype = FindBaseType(descs[0]);
	}
	else if (descs.size() == 2)
	{
		basetype = FindBaseType(descs[0]);
		buftype = FindBufferType(descs[1]);
	}
	else if (descs.size() > 2)
	{
		basetype = FindBaseType(descs[0]);
		buftype = FindBufferType(descs[1]);
		sizedesc = descs[2];
	}

	if (buftype != ParameterBufferType::None)
	{
		IBufferPtr val = nullptr;
		if (TryParseBuffer(basetype, buftype, sizedesc, valexpr.substr(val_startpos), val))
		{
			buffers.insert(std::make_pair(InertiaString{ name }, val));
			indices.insert(std::make_pair((int)indices.size(), InertiaString{ name }));
			return true;
		}
		else
		{
			err = L"can not parse buffer: " + name;
			return false;
		}
	}
	else
	{
		boost::any val;
		if (TryParse(basetype, valexpr.substr(val_startpos), val))
		{
			params.insert(std::make_pair(InertiaString{ name }, val));
			indices.insert(std::make_pair((int)indices.size(), InertiaString{ name }));
			return true;
		}
		else
		{
			err = L"can not parse value: " + name;
			return false;
		}
	}
	return false;
}

bool StringDerivedParameterPacket::TryAddByString(const std::wstring& name, const std::wstring& valexpr, std::wstring& err)
{
	if (valexpr[0] != L'[')
	{
		err = L"invalid value expression: '['";
		return false;
	}
	size_t basebrace = valexpr.find_first_of(L']');
	if (basebrace == valexpr.npos)
	{
		err = L"invalid value expression: ']'";
		return false;
	}
	std::wstring typedesc = valexpr.substr(1, basebrace - 1);
	if (basebrace > valexpr.size() - 2)
	{
		err = L"invalid value expression: empty value";
		return false;
	}

	std::vector<std::wstring> descs;
	boost::split(descs, typedesc, boost::is_any_of(L";"));

	ParameterBaseType basetype = ParameterBaseType::None;
	ParameterBufferType buftype = ParameterBufferType::None;
	std::wstring sizedesc;
	size_t val_startpos = basebrace + 1;

	if (descs.size() == 1)
	{
		basetype = FindBaseType(descs[0]);
	}
	else if (descs.size() == 2)
	{
		basetype = FindBaseType(descs[0]);
		buftype = FindBufferType(descs[1]);
	}
	else if (descs.size() > 2)
	{
		basetype = FindBaseType(descs[0]);
		buftype = FindBufferType(descs[1]);
		sizedesc = descs[2];
	}

	if (buftype != ParameterBufferType::None)
	{
		IBufferPtr val = nullptr;
		if (TryParseBuffer(basetype, buftype, sizedesc, valexpr.substr(val_startpos), val))
		{
			buffers.insert(std::make_pair(InertiaString{ name }, val));
			indices.insert(std::make_pair((int)indices.size(), InertiaString{ name }));
			return true;
		}
		else
		{
			err = L"can not parse buffer: " + name;
			return false;
		}
	}
	else
	{
		boost::any val;
		if (TryParse(basetype, valexpr.substr(val_startpos), val))
		{
			params.insert(std::make_pair(InertiaString{ name }, val));
			indices.insert(std::make_pair((int)indices.size(), InertiaString{ name }));
			return true;
		}
		else
		{
			err = L"can not parse value: " + name;
			return false;
		}
	}
	return false;
}

#pragma endregion

#pragma region ParameterStringReader
ParameterPacketPtr ParameterStringReader::CreateFromString(const std::wstring& strings, std::wstring& err)
{
	StringDerivedParameterPacketPtr packet = std::make_shared<StringDerivedParameterPacket>();
	std::wistringstream buffer(strings);
	std::wstring line;
	while (std::getline(buffer, line))
	{
		if (!packet->TryAddByString(line, err))
		{
			return nullptr;
		}
	}
	return packet;
}

bool ParameterStringReader::AttachTo(ParameterPacketPtr param, const std::wstring& strings, std::wstring& err)
{
	StringDerivedParameterPacketPtr packet = std::dynamic_pointer_cast<StringDerivedParameterPacket>(param);
	if (!packet) return false;
	std::wistringstream buffer(strings);
	std::wstring line;
	while (std::getline(buffer, line))
	{
		if (!packet->TryAddByString(line, err))
		{
			return false;
		}
	}
	return true;
}

ParameterPacketPtr ParameterStringReader::CreateFromString(const std::wstring& paramname, const std::wstring& valexpression, std::wstring& err)
{
	StringDerivedParameterPacketPtr packet = std::make_shared<StringDerivedParameterPacket>();
	if (!packet->TryAddByString(paramname, valexpression, err))
	{
		return nullptr;
	}
	return packet;
}

bool ParameterStringReader::AttachTo(ParameterPacketPtr params, const std::wstring& paramname, const std::wstring& valexpression, std::wstring& err)
{
	StringDerivedParameterPacketPtr packet = std::dynamic_pointer_cast<StringDerivedParameterPacket>(params);
	if (!packet) return false;
	return packet->TryAddByString(paramname, valexpression, err);
}

#pragma endregion