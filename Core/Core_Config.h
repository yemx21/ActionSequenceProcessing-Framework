#pragma once
#ifndef CORE_CAPI
#ifdef CORE_EXPORTS
#define CORE_CAPI extern "C" __declspec(dllexport)
#else
#define CORE_CAPI extern "C" __declspec(dllimport)
#endif
#endif

#ifndef CORE_API
#ifdef CORE_EXPORTS
#define CORE_API __declspec(dllexport)
#else
#define CORE_API __declspec(dllimport)
#endif
#endif

typedef float datatype;
typedef __int64 sizetype;
typedef unsigned __int64 usizetype;

#define ENUM_OPERATORS(T) \
enum class T;	\
	inline T	operator	&	(T x, T y) { return static_cast<T>	(static_cast<intptr_t>(x)& static_cast<intptr_t>(y)); }; \
	inline T	operator	|	(T x, T y) { return static_cast<T>	(static_cast<intptr_t>(x) | static_cast<intptr_t>(y)); }; \
	inline T	operator	^	(T x, T y) { return static_cast<T>	(static_cast<intptr_t>(x) ^ static_cast<intptr_t>(y)); }; \
	inline T	operator	~	(T x) { return static_cast<T>	(~static_cast<intptr_t>(x)); }; \
	inline T&	operator	&=	(T& x, T y) { x = x & y;	return x; }; \
	inline T&	operator	|=	(T& x, T y) { x = x | y;	return x; }; \
	inline T&	operator	^=	(T& x, T y) { x = x ^ y;	return x; }; 

#define ENUM_FLAGS(T) ENUM_OPERATORS(T)


#define ENUM_OPERATORSEX(T, TYPE) \
enum class T: TYPE;	\
	inline T	operator	&	(T x, T y) { return static_cast<T>	(static_cast<TYPE>(x)& static_cast<TYPE>(y)); }; \
	inline T	operator	|	(T x, T y) { return static_cast<T>	(static_cast<TYPE>(x) | static_cast<TYPE>(y)); }; \
	inline T	operator	^	(T x, T y) { return static_cast<T>	(static_cast<TYPE>(x) ^ static_cast<TYPE>(y)); }; \
	inline T	operator	~	(T x) { return static_cast<T>	(~static_cast<TYPE>(x)); }; \
	inline T&	operator	&=	(T& x, T y) { x = x & y;	return x; }; \
	inline T&	operator	|=	(T& x, T y) { x = x | y;	return x; }; \
	inline T&	operator	^=	(T& x, T y) { x = x ^ y;	return x; }; 

#define ENUM_FLAGSEX(T, TYPE) ENUM_OPERATORSEX(T, TYPE)

class CORE_API BenchMarkTimer
{
	__int64   Frequency;
	__int64   startTime;
	__int64   endTime;
public:
	BenchMarkTimer();

	void ReStart();
	void Stop();
	double GetDuration();
};

#ifndef ZEROSTRUCT
#define ZEROSTRUCT
template <typename T>
class ZeroStruct
{
public:
	ZeroStruct() { memset(this, 0, sizeof(T)); }
};
#endif

#ifndef SAFERELEASE
#define SAFERELEASE
template<class Interface>
inline void SafeRelease(Interface *& pInterfaceToRelease)
{
	if (pInterfaceToRelease != 0)
	{
		pInterfaceToRelease->Release();
		pInterfaceToRelease = 0;
	}
}
#endif

#ifndef SAFEDELETE
#define SAFEDELETE
template<class Pointer>
inline void SafeDelete(Pointer *& pointerToDelete)
{
	if (pointerToDelete)
	{
		delete pointerToDelete;
		pointerToDelete = nullptr;
	}
}

template<class Pointer>
inline void SafeDeleteRef(void *& pointerToDelete)
{
	Pointer* cptr = (Pointer*)pointerToDelete;
	if (cptr)
	{
		delete cptr;
		cptr = nullptr;
		pointerToDelete = nullptr;
	}
}

template<class Pointer>
inline void SafeFree(Pointer *& pointerToDelete)
{
	if (pointerToDelete)
	{
		free(pointerToDelete);
		pointerToDelete = nullptr;
	}
}

template<class Pointer>
inline void SafeFreeRef(void *& pointerToDelete)
{
	Pointer* cptr = (Pointer*)pointerToDelete;
	if (cptr)
	{
		free(cptr);
		cptr = nullptr;
		pointerToDelete = nullptr;
	}
}

template<class Pointer>
inline void SafeDelete1(Pointer *& pointerToDelete)
{
	if (pointerToDelete)
	{
		delete[] pointerToDelete;
		pointerToDelete = nullptr;
	}
}
#endif

#define DISABLE_COPY_AND_ASSIGN(classname) \
private:\
  classname(const classname&);\
  classname& operator=(const classname&)

#ifndef RUNTIMEENUM
#define RUNTIMEENUM

#include <type_traits>
#include <string>
#include <vector>

namespace rsdn
{
	struct Enums
	{
		template<typename T>
		static typename std::enable_if<std::is_enum<T>::value, bool>::type Contains(T x, T testFlag)
		{
			typedef typename std::underlying_type<T>::type base;

			return static_cast<base>(x & testFlag) == static_cast<base>(testFlag);
		}
	};

	enum class ComputationMode { CPU, GPU };

	class CORE_API Runtime
	{
	private:
		ComputationMode mode;
		int cpujobcount;
		std::vector<int> gpus;
		Runtime();
	public:
		static Runtime& Get();

		ComputationMode GetMode();
		void SetMode(ComputationMode);

		int GetCpuJobCount();
		void SetCpuJobCount(int);

		const std::vector<int>& GetGpuDevices();

		inline static ComputationMode Mode() { return Get().GetMode(); }
		inline static int CpuJobCount() { return Get().GetCpuJobCount(); }

		static bool Load(const std::wstring& dll);
	};

	class CORE_API Random
	{
	public:
		static void Initilize();
		static void Shutdown();

		static int Generate(int min, int max);

		static int Generate(bool positive = true);
	};

	enum class LogLevel
	{
		Info,
		Warn,
		Error
	};

	class Logger_impl;
	class CORE_API Logger
	{
	private:
		Logger_impl* impl;
		friend Logger_impl;
		Logger();
	public:
		~Logger();
		static Logger& Get();
		bool Init(const std::wstring& logfile);
	
		Logger& Report(LogLevel lvl);
		

		static Logger& endl(Logger& _Ostr);

		Logger& operator << (char c);
		Logger& operator << (const char* p);
		Logger& operator << (const std::string& p);

		Logger& operator << (wchar_t c);
		Logger& operator << (const wchar_t* p);
		Logger& operator << (const std::wstring& p);

		Logger& operator << (short val);
		Logger& operator << (unsigned short val);

		Logger& operator << (int val);
		Logger& operator << (unsigned int val);

		Logger& operator << (long long val);
		Logger& operator << (unsigned long long val);

		Logger& operator << (float val);
		Logger& operator << (double val);
		Logger& operator << (long double val);
		Logger& operator << (const void* val);

		Logger& operator << (Logger& (__cdecl *_Pfn)(Logger&));
	};
}
#endif

/// <summary>
/// seek position origin
/// </summary>
enum class SeekOrigin
{
	Begin,
	Current,
	End,
};

namespace rsdn
{
	class Object;

	class Type;
}

typedef const rsdn::Type* _type;

#define __object  public rsdn::Object
#define __vobject  virtual public rsdn::Object

#define RSDN_MAX(x, y) (((x) > (y)) ? (x) : (y))
#define RSDN_MIN(x, y) (((x) < (y)) ? (x) : (y))