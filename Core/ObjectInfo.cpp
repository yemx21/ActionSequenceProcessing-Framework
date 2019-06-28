#include "ObjectInfo.h"
#include "ObjectInfo_impl.h"
#include "InertiaString.h"
#include <string>
#include <vector>
#include <set>
#include <exception>
#include <memory>
#include <assert.h>
#include <mutex>
using namespace std;
using namespace rsdn;


namespace rsdn
{
	class BaseObjectEnumerator_impl
	{
	protected:
		friend class ObjectInfo;
		friend class BaseObjectEnumerator;
		typedef set<const ObjectInfo*> base;
		typedef set<const ObjectInfo*>::const_iterator base_iterator;
		typedef set<const ObjectInfo*>::size_type pos_type;
		base iterOwner;
		base_iterator citer;
		std::mutex lock;
		BaseObjectEnumerator_impl(const ObjectInfo_internal* cls)
		{
			for (auto& b : *cls->bcls)
			{
				if (iterOwner.find(b) != iterOwner.end()) continue;
				iterOwner.insert(b);
				ObjectInfo_internal::LoopCountBaseObject(&iterOwner, b);
			}
			citer = iterOwner.begin();
		}

		unsigned int Count() const
		{
			return iterOwner.size();
		}

		const wchar_t* GetCurrentName() const
		{
			const ObjectInfo* m = *citer;
			return m != nullptr ? m->Name : nullptr;
		}

		const ObjectInfo* GetCurrent() const
		{
			return *citer;
		}

		void Next()
		{
			if (citer == iterOwner.end()) return;
			lock_guard<mutex> locker(lock);
			citer++;
		}

		void Previous()
		{
			if (citer == iterOwner.begin()) return;
			lock_guard<mutex> locker(lock);
			citer--;
		}

		bool IsEnd() const
		{
			return citer == iterOwner.end();
		}
	};

	BaseObjectEnumerator::BaseObjectEnumerator() :impl(nullptr)
	{

	}

	BaseObjectEnumerator::~BaseObjectEnumerator()
	{
		if (impl) { delete impl; impl = nullptr; }
	}

	const wchar_t* BaseObjectEnumerator::GetCurrentName() const
	{
		if (impl) return impl->GetCurrentName();
		return nullptr;
	}

	const ObjectInfo* BaseObjectEnumerator::GetCurrent() const
	{
		if (impl) return impl->GetCurrent();
		return nullptr;
	}

	unsigned int BaseObjectEnumerator::GetCount() const
	{
		return impl != nullptr ? impl->Count() : 0;
	}

	void BaseObjectEnumerator::Next()
	{
		if (impl) impl->Next();
	}

	void BaseObjectEnumerator::Previous()
	{
		if (impl) impl->Previous();
	}

	bool BaseObjectEnumerator::GetIsEnd() const
	{
		if (impl) return impl->IsEnd();
		return true;
	}
}

void ObjectInfo::Create(Type const*& pointer, DefineMetaFunc func, Objectctorfunc ctor, const wchar_t* name, unsigned int size)
{
	if (pointer != nullptr)return;
	ObjectInfo* type = new ObjectInfo(name, size);
	type->internal->ctor = ctor;
	pointer = type;
	assert(func != nullptr);
	func(*type);
}


ObjectInfo::ObjectInfo(const wchar_t* name, unsigned int size) : Type(TypeTag::Object)
{
	impl = new ObjectInfo_impl();
	impl->oname = name;
	impl->size = size;

	internal = new ObjectInfo_internal();
	internal->bcls = new std::set<const ObjectInfo*>();
}

ObjectInfo::~ObjectInfo()
{
	SafeDelete(impl);
	SafeDelete(internal);
}

unique_ptr<BaseObjectEnumerator> ObjectInfo::GetBaseObjectEnumerator() const
{
	BaseObjectEnumerator* miter = new BaseObjectEnumerator();
	miter->impl = new BaseObjectEnumerator_impl(internal);
	return unique_ptr<BaseObjectEnumerator>(miter);
}

ObjectInfoError ObjectInfo::AddBaseObject(_type type)
{
	try
	{
		if (!type) return ObjectInfoError::EmptyArgument;
		if (type->Tag != TypeTag::Object) return ObjectInfoError::InvalidArgument;
		const ObjectInfo* cls = dynamic_cast<const ObjectInfo*>(type);
		if (!cls) return ObjectInfoError::InvalidArgument;
		if (internal)
		{
			if (cls == this) return ObjectInfoError::AlreadyExisted;
			for (auto& b : *internal->bcls)
			{
				if (b->NumberOfBaseObjectes > 0)
				{
					if (b->IsBaseOf(cls)) return ObjectInfoError::AlreadyExisted;
				}
				else
				{
					if (b == cls) return ObjectInfoError::AlreadyExisted;
				}
			}
			auto res = internal->bcls->insert(cls);
			if (!res.second)
			{
				return ObjectInfoError::AlreadyExisted;
			}
			return ObjectInfoError::OK;
		}
	}
	catch (...)
	{
		return ObjectInfoError::Unknown;
	}
	return ObjectInfoError::OK;
}

const wchar_t* ObjectInfo::GetName() const
{
	if (internal &&impl)
	{
		return impl->oname->c_str();
	}
	return nullptr;
}

unsigned int ObjectInfo::GetNumberOfBaseObjectes()const
{
	if (internal)
		if (internal->bcls)
		{
			set<const ObjectInfo*> all;
			for (auto& b : *internal->bcls)
			{
				if (all.find(b) != all.end()) continue;
				all.insert(b);
				ObjectInfo_internal::LoopCountBaseObject(&all, b);
			}
			return all.size();
		}
	return 0;
}

bool ObjectInfo::IsBaseOf(_type info)const
{
	try
	{
		const ObjectInfo* cls = dynamic_cast<const ObjectInfo*>(info);
		if (cls)
		{
			if (internal)
			{
				if (internal->bcls)
				{
					for (auto& b : *internal->bcls)
					{
						if (b->NumberOfBaseObjectes > 0)
						{
							if (b == info) return true;
							if (b->IsBaseOf(info)) return true;
						}
						else
						{
							if (b == info) return true;
						}
					}
				}
			}
		}
	}
	catch (...) {}
	return false;
}

unsigned int ObjectInfo::GetSize() const
{
	if (impl) return impl->size;
	return 0;
}

