#include "TypeFactory_impl.h"
#include "Type.h"
#include <algorithm>
using namespace rsdn;

namespace rsdn
{
	class TypeComparer : public std::unary_function<_type, bool>
	{
	private:
		const wchar_t* name;
	public:
		explicit TypeComparer(_type cls)
		{
			name = cls != NULL ? cls->Name : 0;
		}
		bool operator() (_type cls)
		{
			return name == nullptr || cls == nullptr ? false : wcscmp(cls->Name, name) == 0;
		}
	};
}


TypeFactory_impl::TypeFactory_impl()
{

}

TypeFactory_impl::~TypeFactory_impl()
{
	for (auto& i : Registry)
	{
		SafeDelete(i);
	}
	Registry.clear();
}

Type* TypeFactory_impl::Regisiter(Type* type)
{
	if (!type) return nullptr;
	if (!type->Name) return nullptr;
	auto iter = std::find_if(Registry.begin(), Registry.end(), TypeComparer(type));
	if (iter != Registry.end()) return type;
	Registry.push_back(type);
	return type;
}