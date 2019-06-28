#pragma once
#include "Core_Config.h"
#include <objbase.h>
#include <algorithm>

namespace rsdn
{
	class TypeCaches_impl;

	class CORE_API TypeCaches
	{
	protected:
		friend class TypeCaches_impl;
		TypeCaches_impl* impl;
	public:
		TypeCaches();
		~TypeCaches();

		bool Insert(_type type);

		void Remove(_type type);

		_type GetItem(size_t index) const;

		size_t GetItemSize() const;

		_declspec(property(get = GetItemSize)) size_t ItemSize;

		_declspec(property(get = GetItem)) _type Items[];
	};
}
