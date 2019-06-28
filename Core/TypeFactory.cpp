#include "TypeFactory.h"
#include "TypeFactory_impl.h"
#include "FileStreamReader.h"
#include "ObjectInfo.h"
#include "ObjectInfo_impl.h"
#include <boost/locale.hpp>
#include <boost/algorithm/string.hpp>
using namespace rsdn;

TypeFactory::TypeFactory()
{
	impl = std::make_shared<TypeFactory_impl>();
}

TypeFactory& TypeFactory::GetInstance()
{
	static TypeFactory instance;
	return instance;
}

rsdn::Type* TypeFactory::Regisiter(rsdn::Type* type)
{
	TypeFactory& instance = TypeFactory::GetInstance();
	if (instance.impl->Regisiter(type)) return type;		
	return nullptr;
}

bool TypeFactory::SetBinder(const std::wstring& path)
{
	try
	{
		TypeFactory& instance = TypeFactory::GetInstance();
		boost::locale::generator gen;
		std::locale sysloc = gen("");
		FileStreamReader reader{ path.c_str(), Encodings::Utf8, sysloc, false, false };
		std::wstring line;
		instance.impl->Table.clear();
		while (!reader.IsEndOfStream())
		{
			 line = reader.ReadLine(false);
			 vector<wstring> strs;
			 boost::split(strs, line, boost::is_any_of(L"="));
			 for (auto& str : strs) boost::trim(str, sysloc);
			 if (strs.size() == 2) instance.impl->Table.insert(std::make_pair(InertiaString{ strs[0] }, InertiaString{ strs[1] }));
		}
	}
	catch (...)
	{
	}
	return false;
}

std::shared_ptr<Object> TypeFactory::CreateInstance(const std::wstring& name)
{
	try
	{
		TypeFactory& instance = TypeFactory::GetInstance();
		auto tableiter = instance.impl->Table.find(InertiaString{ name });
		if (tableiter != instance.impl->Table.end())
		{
			std::wstring found = tableiter->second;
			auto founditer = std::find_if(instance.impl->Registry.begin(), instance.impl->Registry.end(), [&found](_type a)
			{
				return found.compare(a->Name) == 0;
			});
			if (founditer != instance.impl->Registry.end())
			{
				const ObjectInfo* cls = dynamic_cast<const ObjectInfo*>(*founditer);
				return cls->internal->ctor();
			}
		}
		else
		{
			auto founditer = std::find_if(instance.impl->Registry.begin(), instance.impl->Registry.end(), [&name](_type a)
			{
				return name.compare(a->Name) == 0;
			});
			if (founditer != instance.impl->Registry.end())
			{
				const ObjectInfo* cls = dynamic_cast<const ObjectInfo*>(*founditer);
				return cls->internal->ctor();
			}
		}
	}
	catch (...)
	{

	}
	return nullptr;
}

bool TypeFactory::LoadAssembly(const std::wstring& path)
{
	auto handle = LoadLibraryW(path.c_str());
	if (handle == NULL) return false;
	return true;
}
