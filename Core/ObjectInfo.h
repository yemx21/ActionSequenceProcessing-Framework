#pragma once 
#include "Type.h"
#include <memory>
#include <functional>
using namespace std;

namespace rsdn
{
	class Object;
	class TypeFactory;

	enum class ObjectInfoError
	{
		OK = 0,
		None = 0,
		AlreadyExisted,
		Unknown,
		EmptyArgument,
		InvalidArgument,
	};

	struct ObjectInfo_impl;

	struct ObjectInfo_internal;

	class BaseObjectEnumerator_impl;

	class CORE_API BaseObjectEnumerator final
	{
	private:
		friend class ObjectInfo;
		BaseObjectEnumerator_impl* impl;
		BaseObjectEnumerator();
	public:
		~BaseObjectEnumerator();

		unsigned int GetCount() const;
		const wchar_t* GetCurrentName() const;
		const ObjectInfo* GetCurrent() const;
		void Next();
		void Previous();
		bool GetIsEnd() const;
		__declspec(property(get = GetCurrentName)) const wchar_t* CurrentName;
		__declspec(property(get = GetCurrent)) const ObjectInfo* Current;
		__declspec(property(get = GetCount)) unsigned int Count;
		__declspec(property(get = GetIsEnd)) bool IsEnd;

		DISABLE_COPY_AND_ASSIGN(BaseObjectEnumerator);
	};

	typedef std::function<std::shared_ptr<Object>()> Objectctorfunc;

	class CORE_API ObjectInfo final: public Type
	{
	private:
		friend rsdn::TypeFactory;
		friend Object;
		friend struct ObjectInfo_internal;

		ObjectInfo_impl* impl;
		ObjectInfo_internal* internal;

		ObjectInfoError AddBaseObject(_type type);
	public:
		typedef void(*DefineMetaFunc)(ObjectInfo& type);
	protected:
		ObjectInfo(const wchar_t* name, unsigned int size);
	public:
		/// <remarks>internal use</remarks>
		static void Create(Type const*& pointer, DefineMetaFunc func, Objectctorfunc ctor, const wchar_t* name, unsigned int size);

		/// <remarks>internal use</remarks>
		template<class T>
		static void Create(Type const*& pointer, const wchar_t* name, unsigned int size)
		{
			Create(pointer, &T::DefineMeta, [] {return std::make_shared<T>(); }, name, size);
		}

		~ObjectInfo();
	public:
		unique_ptr<BaseObjectEnumerator> GetBaseObjectEnumerator() const;

		const wchar_t* GetName() const;
		unsigned int GetSize() const;
		unsigned int GetNumberOfBaseObjectes()const;
		bool IsBaseOf(_type info)const;

		__declspec(property(get = GetName)) const wchar_t* Name;
		__declspec(property(get = GetNumberOfBaseObjectes)) unsigned int NumberOfBaseObjectes;

		template<typename T>
		ObjectInfoError AddBaseObject()
		{
			_type ty = TypeUnpacker::Create<T>();
			if (ty) return AddBaseObject(ty);
			return ObjectInfoError::InvalidArgument;
		}

	};
}
