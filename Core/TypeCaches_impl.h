#pragma once
#include "Type.h"
#include <vector>
#include <objbase.h>
#include <algorithm>
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

	class TypeCaches_impl
	{
	protected:
		friend class TypeCaches;
		std::vector<_type> Registry;

		TypeCaches_impl()
		{
		}

		~TypeCaches_impl()
		{
			Registry.clear();
		}

		bool Insert(_type type)
		{
			if (!type) return false;
			if (!type->Name) return false;
			auto iter = std::find_if(Registry.begin(), Registry.end(), TypeComparer(type));
			if (iter != Registry.end()) return false;
			Registry.push_back(type);
			return true;
		}

		void Remove(_type type)
		{
			if (!type) return;
			auto iter = std::find_if(Registry.begin(), Registry.end(), TypeComparer(type));
			if (iter != Registry.end()) Registry.erase(iter);
		}

		size_t GetItemSize() const
		{
			return Registry.size();
		}

		_type GetItem(size_t index) const
		{
			if (index >= 0 && index < Registry.size())
			{
				return Registry.at(index);
			}
			return nullptr;
		}
	};
}
