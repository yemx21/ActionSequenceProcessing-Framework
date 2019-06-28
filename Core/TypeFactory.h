#pragma once
#include "Core_Config.h"
#include "TypeList.h"
#include <memory>
namespace rsdn
{
	class TypeFactory_impl;

	///<remarks>type regisiter</remarks>
	class CORE_API TypeFactory
	{
	private:
		friend TypeFactory_impl;
		std::shared_ptr<TypeFactory_impl> impl;
		TypeFactory();
	protected:
		static TypeFactory& GetInstance();
	public:
		static rsdn::Type* Regisiter(rsdn::Type* type);
		static bool SetBinder(const std::wstring& path);
		static std::shared_ptr<Object> CreateInstance(const std::wstring& name);

		template<typename T>
		static std::shared_ptr<T> CreateInstance(const std::wstring& name)
		{
			return std::dynamic_pointer_cast<T>(CreateInstance(name));
		}

		static bool LoadAssembly(const std::wstring& path);
	};

	class CORE_API TypeUnpacker
	{
	public:
		template<class T, bool SUPPORTED = is_TypeHelper_supported<T>::value>
		struct Resolve;

		template<class T>
		struct Resolve<T, false>
		{
			static_assert(!is_template<T>::value || rsdn::is_container<T>::value || rsdn::is_memory_ptr<T>::value, "T is not defined, its defination is essential. Use Object as its base type");
			static _type Get()
			{
				return Type::unknownType;
			}
		};

		template<class T>
		struct Resolve<T, true>
		{
			static _type Get()
			{
				return TypeHelper<T>::GetType();
			}
		};

	public:

		template<class T>
		static _type Create()
		{
			return Resolve<typename deduce<T>::type>::Get();
		}

	};

}

#define _typeof(OBJECT)  rsdn::TypeUnpacker::Create<OBJECT>()
