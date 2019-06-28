#pragma once
#include "Core_Config.h"
#include <Windows.h>
namespace rsdn
{
	class CORE_API CriticalSection
	{
	private:
		CRITICAL_SECTION cs;
		CriticalSection(const CriticalSection&) = delete;
		CriticalSection& operator=(const CriticalSection&) = delete;
	public:
		CriticalSection()
		{
			::InitializeCriticalSection(&cs);
		}

		~CriticalSection()
		{
			::DeleteCriticalSection(&cs);
		}

		bool TryEnter()
		{
			return (bool)(::TryEnterCriticalSection(&cs) != 0);
		}

		void Enter()
		{
			EnterCriticalSection(&cs);
		}

		void Leave()
		{
			LeaveCriticalSection(&cs);
		}
	};

}

#include <mutex>

namespace std
{
	template<>
	class lock_guard<rsdn::CriticalSection>
	{
	public:
		typedef rsdn::CriticalSection mutex_type;

		explicit lock_guard(rsdn::CriticalSection& _Mtx)
			: _Mutex(_Mtx)
		{
			_Mutex.Enter();
		}

		lock_guard(rsdn::CriticalSection& _Mtx, adopt_lock_t)
			: _Mutex(_Mtx)
		{
		}

		~lock_guard() _NOEXCEPT
		{
			_Mutex.Leave();
		}

		lock_guard(const lock_guard&) = delete;
		lock_guard& operator=(const lock_guard&) = delete;

	private:
		rsdn::CriticalSection& _Mutex;
	};

}
