#pragma once
#include "Core_Config.h"
#include <exception>

namespace rsdn
{
	enum class OSVersion
	{
		XP_0 = 10,
		XP_1,
		XP_2,
		XP_3,

		Vista_0 = 20,
		Vista_1,
		Vista_2,

		Win7_0 = 30,
		Win7_1,

		Win8_0 = 40,
		Win8_1,


		UnSupported = 0x10000,
	};

	class CORE_API safe_void_ptr final
	{
	private:
		void* _ptr;
	public:
		safe_void_ptr();
		safe_void_ptr(void* ptr);
		safe_void_ptr(const safe_void_ptr& ptr);
		safe_void_ptr(safe_void_ptr&& ptr);
		~safe_void_ptr();
		safe_void_ptr& operator = (void* newPtr);
		safe_void_ptr& operator = (const safe_void_ptr& ptr);
		void* get(unsigned int checkSize) const;
		size_t getHash() const;
		bool operator == (const safe_void_ptr& ptr) { return _ptr == ptr._ptr; }
		operator bool() const { return _ptr != nullptr; }
	};

	template<typename T, size_t N>
	struct add_pointer_ex
	{
		typedef typename add_pointer_ex<T*, N - 1>::type type;
	};

	template<typename T>
	struct add_pointer_ex<T, 0U>
	{
		typedef typename T type;
	};

	class CORE_API OSHelper
	{
		template<typename T, size_t START, size_t END, bool SAME>
		struct AddressChecker
		{
			__inline static bool Check(typename add_pointer_ex<T, START>::type mem, bool readWrite = true)
			{
				if (!OSHelper::IsValidAddress(mem, sizeof(deduce<T>::type), readWrite)) return false;
				return AddressChecker<T, START - 1, END, START - 1 == END>::Check(mem[0], readWrite);
			}
		};

		template<typename T, size_t START, size_t END>
		struct AddressChecker<T, START, END, true>
		{
			__inline static bool Check(typename add_pointer_ex<T, START>::type mem, bool readWrite = true)
			{
				return OSHelper::IsValidAddress(mem, sizeof(deduce<T>::type), readWrite);
			}
		};

	public:
		static std::exception GetErrorException(unsigned long errorCode);

		static std::exception GetLastErrorException();

		static bool IsValidAddress(const void* lp, unsigned int nBytes, bool readWrite = true);

		template<typename T, size_t START, size_t END>
		__inline static bool IsValidAddress(typename add_pointer_ex<T, START>::type mem, bool readWrite = true)
		{
			return AddressChecker<T, START, END, START == END>::Check(mem, readWrite);
		}

		static OSVersion GetOSVersion();
	
	};
}

#include <xhash>
namespace std
{
	template <>
	struct hash<rsdn::safe_void_ptr>
	{
		inline size_t operator()(const rsdn::safe_void_ptr& obj) const
		{
			return obj.getHash();
		}
	};
}
