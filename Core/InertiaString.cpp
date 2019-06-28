#include "InertiaString.h"
#include <unordered_map>
#include <thread>
#include "CriticalSection.h"
using namespace rsdn;

struct InertiaStringCache
{
	typedef unordered_map<wstring, std::weak_ptr<const wstring>> container_type;
private:
	container_type container;
	CriticalSection mutex;
	InertiaStringCache() = default;
	~InertiaStringCache() = default;
public:
	InertiaStringCache(InertiaStringCache const&) = delete;
	InertiaStringCache(InertiaStringCache&&) = delete;

	InertiaStringCache& operator = (InertiaStringCache const&) = delete;
	InertiaStringCache& operator = (InertiaStringCache&&) = delete;

	static InertiaStringCache& ref() throw();

	std::shared_ptr<const wstring> find(const wstring& value) throw();

private:
	std::shared_ptr<const wstring> insert(const wstring& value) throw();

	static void remove(wstring const* ptr) throw();

};

InertiaStringCache& InertiaStringCache::ref() throw()
{
	static InertiaStringCache instance;
	return instance;
}

std::shared_ptr<const wstring> InertiaStringCache::find(const wstring& value) throw()
{
	std::lock_guard<CriticalSection> lock{ this->mutex };
	auto iter = this->container.find(value);
	if (iter != this->container.end()) { return iter->second.lock(); }
	return insert(value);
}

std::shared_ptr<const wstring> InertiaStringCache::insert(const wstring& value) throw()
{
	auto result = this->container.emplace(value, std::shared_ptr<add_const_t<wstring>> { });
	if (!std::get<1>(result)) {}
	auto iter = std::get<0>(result);
	auto const& key = std::get<0>(*iter);
	std::shared_ptr<add_const_t<wstring>> shared{ std::addressof(key), remove };
	this->container[key] = shared;
	return shared;
}

void InertiaStringCache::remove(wstring const* ptr) throw()
{
	if (!ptr) { return; }
	InertiaStringCache& cache = ref();
	try
	{
		std::lock_guard<CriticalSection> lock{ cache.mutex };
		cache.container.erase(*ptr);
	}
	catch (...)
	{
	}
}


InertiaString::InertiaString(const wstring& str) : handle{ InertiaStringCache::ref().find(str) }
{

}

InertiaString::InertiaString(const wchar_t* str) : handle{ InertiaStringCache::ref().find(str) }
{
}

InertiaString::InertiaString(const wchar_t* str, size_t len) : handle{ InertiaStringCache::ref().find(len == 0 ? wstring() : wstring(str, len)) }
{

}

InertiaString::InertiaString() : handle{ InertiaStringCache::ref().find(wstring()) }
{

}


const wstring* InertiaString::operator -> () const throw()
{
	return this->handle.get();
}

InertiaString::operator const wstring&() const throw()
{
	return *this->handle;
}

const wstring& InertiaString::get() const throw()
{
	return *this->handle;
}

void InertiaString::swap(InertiaString& that) throw()
{
	std::swap(this->handle, that.handle);
}

namespace rsdn
{
	bool operator == (InertiaString const& lhs, InertiaString const& rhs)
	{
		return std::equal_to<wstring> { }(lhs, rhs);
	}

	bool operator != (InertiaString const& lhs, InertiaString const& rhs)
	{
		return std::not_equal_to<wstring> { }(lhs, rhs);
	}

	bool operator >= (InertiaString const& lhs, InertiaString const& rhs)
	{
		return std::greater_equal<wstring> { }(lhs, rhs);
	}

	bool operator <= (InertiaString const& lhs, InertiaString const& rhs)
	{
		return std::less_equal<wstring> { }(lhs, rhs);
	}

	bool operator > (InertiaString const& lhs, InertiaString const& rhs)
	{
		return std::greater<wstring> { }(lhs, rhs);
	}

	bool operator < (InertiaString const& lhs, InertiaString const& rhs)
	{
		return std::less<wstring> { }(lhs, rhs);
	}

	bool operator == (InertiaString const& lhs, wstring const& rhs)
	{
		return std::equal_to<wstring> { }(lhs, rhs);
	}

	bool operator != (InertiaString const& lhs, wstring const& rhs)
	{
		return std::not_equal_to<wstring> { }(lhs, rhs);
	}

	bool operator >= (InertiaString const& lhs, wstring const& rhs)
	{
		return std::greater_equal<wstring> { }(lhs, rhs);
	}

	bool operator <= (InertiaString const& lhs, wstring const& rhs)
	{
		return std::less_equal<wstring> { }(lhs, rhs);
	}

	bool operator > (InertiaString const& lhs, wstring const& rhs)
	{
		return std::greater<wstring> { }(lhs, rhs);
	}

	bool operator < (InertiaString const& lhs, wstring const& rhs)
	{
		return std::less<wstring> { }(lhs, rhs);
	}
}