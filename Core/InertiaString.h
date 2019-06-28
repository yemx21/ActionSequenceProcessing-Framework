#pragma once
#ifndef SYSTEM_INERTIASTRING
#define SYSTEM_INERTIASTRING

#include "Core_Config.h"
#include <memory>
#include <string>
#include <mutex>
using namespace std;
namespace rsdn
{
	/// <remarks>global unique string, suitable for long life-time short string, used for optimizing string memory pool</remarks>
	class CORE_API InertiaString
	{
	private:
		shared_ptr<const wstring> handle;
	public:
		InertiaString(const wstring& str);

		InertiaString(const wchar_t* str);

		InertiaString(const wchar_t* str, size_t len);

		InertiaString(InertiaString const&) = default;

		InertiaString();

		~InertiaString() = default;

		InertiaString& operator = (InertiaString const&) = default;
		const wstring* operator -> () const throw();
		operator const wstring&() const throw();
		const wstring& get() const throw();
		void swap(InertiaString&) throw();

		template <class ValueType, class = enable_if_t<!std::is_same<InertiaString, decay_t<ValueType>>::value>>
		InertiaString& operator = (ValueType&& value)
		{
			InertiaString{ std::forward<ValueType>(value) }.swap(*this);
			return *this;
		}
	};
	CORE_API bool operator == (rsdn::InertiaString const& lhs, rsdn::InertiaString const& rhs);

	CORE_API bool operator != (rsdn::InertiaString const& lhs, rsdn::InertiaString const& rhs);

	CORE_API bool operator >= (rsdn::InertiaString const& lhs, rsdn::InertiaString const& rhs);

	CORE_API bool operator <= (rsdn::InertiaString const& lhs, rsdn::InertiaString const& rhs);

	CORE_API bool operator > (rsdn::InertiaString const& lhs, rsdn::InertiaString const& rhs);

	CORE_API bool operator < (rsdn::InertiaString const& lhs, rsdn::InertiaString const& rhs);

	CORE_API bool operator == (rsdn::InertiaString const& lhs, wstring const& rhs);

	CORE_API bool operator != (rsdn::InertiaString const& lhs, wstring const& rhs);

	CORE_API bool operator >= (rsdn::InertiaString const& lhs, wstring const& rhs);

	CORE_API bool operator <= (rsdn::InertiaString const& lhs, wstring const& rhs);

	CORE_API bool operator > (rsdn::InertiaString const& lhs, wstring const& rhs);

	CORE_API bool operator < (rsdn::InertiaString const& lhs, wstring const& rhs);
}


namespace std
{
	template <>
	struct hash<rsdn::InertiaString>
	{
		hash<std::wstring>::result_type operator ()(rsdn::InertiaString const& value) const throw()
		{
			return hash<std::wstring> { }(value);
		}
	};

}

#endif
