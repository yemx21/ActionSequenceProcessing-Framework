#include "Type.h"
#include <unordered_map>
#include <unordered_set>
#include <Windows.h>
#include "ObjectInfo.h"
#include "TypeFactory.h"
using namespace std;
using namespace rsdn;

#pragma init_seg( ".CRT$XCC" )

const wchar_t* TypeTag_Name_unknown = L"???";
const wchar_t* TypeTag_Name_object = L"object";

#pragma data_seg("GLOBAL_RSDN_APP") 
volatile unsigned __int64 TYPE_COUNT = 0U;
#pragma data_seg()
#pragma comment(linker,"/section:GLOBAL_RSDN_APP,rws")

namespace rsdn
{
	class Type_impl
	{
	public:
		static HANDLE locker;
	public:
		static Type* Create(TypeTag tag)
		{
			std::hash<std::wstring> hash_fn;
			Type* type = new Type(tag);
			type->_hashCode = hash_fn(type->GetName());
			return type;
		}
		static void EnsureMutex()
		{
			if (!Type_impl::locker)	Type_impl::locker = CreateMutexW(NULL, FALSE, L"Global\\TYPE_LOCKER");
		}
	};

	HANDLE Type_impl::locker = nullptr;
}

Type* const Type::unknownType = TypeFactory::Regisiter(Type_impl::Create(TypeTag::Unknown));
Type* const Type::objectType = TypeFactory::Regisiter(Type_impl::Create(TypeTag::Object));

Type::Type(TypeTag tag) : _tag(tag)
{
	Type_impl::EnsureMutex();
	DWORD dwWaitResult = WaitForSingleObject(Type_impl::locker, INFINITE);

	if (dwWaitResult == WAIT_OBJECT_0 || dwWaitResult == WAIT_ABANDONED)
	{
		if (dwWaitResult == WAIT_ABANDONED)
		{

		}
		TYPE_COUNT++;
		ReleaseMutex(Type_impl::locker);
	}
}

Type::~Type()
{
	Type_impl::EnsureMutex();
	DWORD dwWaitResult = WaitForSingleObject(Type_impl::locker, INFINITE);

	if (dwWaitResult == WAIT_OBJECT_0 || dwWaitResult == WAIT_ABANDONED)
	{
		if (dwWaitResult == WAIT_ABANDONED)
		{

		}
		TYPE_COUNT--;
		ReleaseMutex(Type_impl::locker);
	}
}

unsigned __int64 Type::GetCount()
{
	unsigned __int64 count = 0U;
	Type_impl::EnsureMutex();
	DWORD dwWaitResult = WaitForSingleObject(Type_impl::locker, INFINITE);

	if (dwWaitResult == WAIT_OBJECT_0 || dwWaitResult == WAIT_ABANDONED)
	{
		if (dwWaitResult == WAIT_ABANDONED)
		{

		}
		count = TYPE_COUNT;
		ReleaseMutex(Type_impl::locker);
	}
	return count;
}

const wchar_t* Type::GetName() const
{
	switch (_tag)
	{
	case TypeTag::Unknown:
		return TypeTag_Name_unknown;
	case TypeTag::Object:
		return TypeTag_Name_object;
	default:
		return 0;
	}
}

size_t Type::GetHashCode() const
{
	if (_hashCode) return *_hashCode; else return 0;
}

TypeTag Type::GetTag() const
{
	return _tag;
}

bool Type::GetIsObject() const
{
	return _tag == TypeTag::Object;
}

bool Type::IsBaseOf(_type info)const
{	
	if (_tag == TypeTag::Object)
	{
		const ObjectInfo* ci = dynamic_cast<const ObjectInfo*>(this);
		if (ci)
		{
			return ci->IsBaseOf(info);
		}
	}
	return false;
}

bool Type::IsBaseOfOrSameAs(_type info) const
{
	if (IsType(info)) return true;
	return IsBaseOf(info);
}

void Type::destroy()
{
	delete this;
}

bool Type::IsType(_type type) const
{
	if (!type) return false;
	if (type->_tag == _tag)
	{
		switch (_tag)
		{
		case TypeTag::Object:
		case TypeTag::Unknown:
			return type == this;
		default:
			return false;
		}
	}
	return false;
}


bool Type::IsConvertible(_type type)const
{
	if (!type) return false;
	if (type->_tag == _tag)
	{
		switch (_tag)
		{
		case TypeTag::Unknown:
			return false;
		case TypeTag::Object:
		{
			const ObjectInfo* at = dynamic_cast<const ObjectInfo*>(type);
			const ObjectInfo* this_at = dynamic_cast<const ObjectInfo*>(this);
			if (at && this_at)
			{
				if (at == this_at) return true;
				return this_at->IsBaseOf(at);
			}
			return false;
		}
		default:
			return false;
		}
	}
	else
		return false;
	return false;
}