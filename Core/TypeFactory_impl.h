#pragma once
#include "Core_Config.h"
#include <vector>
#include <string>
#include <map>
#include "InertiaString.h"
namespace rsdn
{
	class TypeFactory_impl
	{
	public:
		std::vector<_type> Registry;
		std::map<InertiaString, InertiaString> Table;

		TypeFactory_impl();

		~TypeFactory_impl();

		Type* Regisiter(Type* type);
	};
}
