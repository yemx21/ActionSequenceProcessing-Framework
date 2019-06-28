#include "Core_Config.h"
#include "cuda_config.h"
#include "Converter.h"
#include "CriticalSection.h"
#include <Windows.h>
#include <boost\thread\tss.hpp>

#include <iostream>
#include <boost/locale/generator.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>

#include <boost/log/common.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/sources/logger.hpp>
#include <boost/log/support/date_time.hpp>

namespace logging = boost::log;
namespace sinks = boost::log::sinks;
namespace attrs = boost::log::attributes;
namespace src = boost::log::sources;
namespace expr = boost::log::expressions;
namespace keywords = boost::log::keywords;

using namespace rsdn;

BenchMarkTimer::BenchMarkTimer() :startTime(0), endTime(0)
{
	QueryPerformanceFrequency((LARGE_INTEGER*)&Frequency);
	Frequency /= 1000;
}

void BenchMarkTimer::ReStart()
{
	QueryPerformanceCounter((LARGE_INTEGER*)&startTime);
}

void BenchMarkTimer::Stop()
{
	QueryPerformanceCounter((LARGE_INTEGER*)&endTime);
}

double BenchMarkTimer::GetDuration()
{
	return (double)(endTime - startTime) / (double)Frequency;
}

#pragma region Runtime

static boost::thread_specific_ptr<Runtime> runtime_instance;

Runtime::Runtime():cpujobcount(1), mode(ComputationMode::CPU)
{
	int numdev = 0;
	cudaGetDeviceCount(&numdev);
	gpus.resize(numdev);
	for (int i = 0; i < numdev; i++)
	{
		cudaDeviceProp prop;
		cudaGetDeviceProperties(&prop, i);
		gpus[i] = i;
	}
}

Runtime& Runtime::Get()
{
	if (!runtime_instance.get()) 
	{
		runtime_instance.reset(new Runtime());
	}
	return *(runtime_instance.get());
}

ComputationMode Runtime::GetMode()
{
	return mode;
}

void Runtime::SetMode(ComputationMode m)
{
	mode = m;
}

int Runtime::GetCpuJobCount()
{
	return cpujobcount;
}

void Runtime::SetCpuJobCount(int val)
{
	cpujobcount = val;
}

const std::vector<int>& Runtime::GetGpuDevices()
{
	return gpus;
}

bool Runtime::Load(const std::wstring& dll)
{
	return LoadLibraryW(dll.c_str()) != NULL;
}

#pragma endregion

#pragma region Random

#pragma comment(lib, "advapi32.lib")
static HCRYPTPROV hProvider = 0;
constexpr DWORD dwLength = sizeof(int);
static BYTE pbBuffer[dwLength] = {};

void Random::Initilize()
{
	if (!hProvider)
	{
		::CryptAcquireContextW(&hProvider, 0, 0, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT | CRYPT_SILENT);
	}
}

void Random::Shutdown()
{
	if (hProvider)
	{
		::CryptReleaseContext(hProvider, 0);
		hProvider = NULL;
	}
}

int Random::Generate(int min, int max)
{
	::CryptGenRandom(hProvider, dwLength, pbBuffer);
	int ret = *(int*)pbBuffer;

	return min + (ret % (max - min));
}

int Random::Generate(bool positive)
{
	::CryptGenRandom(hProvider, dwLength, pbBuffer);
	int ret = *(int*)pbBuffer;
	return positive ? (int)abs(ret) : ret;
}

#pragma endregion

#pragma region Logger

BOOST_LOG_ATTRIBUTE_KEYWORD(Logger_timestamp, "TimeStamp", boost::posix_time::ptime)

namespace rsdn
{
	class Logger_impl
	{
	public:
		bool isbinded;
		CriticalSection locker;
		src::wseverity_logger<boost::log::trivial::severity_level> slg;
		boost::log::trivial::severity_level curlvl;
		boost::log::v2s_mt_nt6::record currec;
		logging::record_ostream curstrm;
		Logger_impl():isbinded(false), curlvl(boost::log::trivial::severity_level::info)
		{

		}

		bool Bind(const std::wstring& path)
		{
			if (isbinded) return false;
			std::string npath = rsdn::Converter::Convert(path);

			boost::shared_ptr<sinks::synchronous_sink< sinks::text_file_backend > > sink = logging::add_file_log
			(
				npath.c_str(),
				keywords::format = expr::stream
				<< expr::format_date_time(Logger_timestamp, "%H:%M:%S.%f")
				<< " <" << boost::log::trivial::severity.or_default(boost::log::trivial::info)
				<< "> " << expr::message
			);

			std::locale loc = boost::locale::generator()("en_US.UTF-8");
			sink->imbue(loc);
			//sink->locked_backend()->auto_flush(true);

			logging::add_console_log(std::cout, 
				keywords::format = expr::stream
				<< expr::format_date_time(Logger_timestamp, "%H:%M:%S.%f")
				<< " <" << boost::log::trivial::severity.or_default(boost::log::trivial::info)
				<< "> " << expr::message,
				keywords::auto_flush = true
			);

			logging::add_common_attributes();
			isbinded = true;

			currec = slg.open_record((keywords::severity = curlvl));
			if (currec)
			{
				logging::record_ostream strm(currec);
				strm << L"logger service opened";
				strm.flush();
				slg.push_record(boost::move(currec));

				currec = slg.open_record((keywords::severity = curlvl));
				curstrm.attach_record(currec);
			}
			return true;
		}
	};
}

Logger::~Logger()
{
	SafeDelete(impl);
}

Logger::Logger()
{
	impl = new rsdn::Logger_impl();
}

Logger& Logger::Get()
{
	static Logger log;
	return log;
}

bool Logger::Init(const std::wstring& logfile)
{
	return impl->Bind(logfile);
}

Logger& Logger::Report(LogLevel lvl)
{
	std::lock_guard<rsdn::CriticalSection> lock(impl->locker);
	auto oldlvl = impl->curlvl;
	switch (lvl)
	{
	case LogLevel::Info:
		impl->curlvl = boost::log::trivial::severity_level::info;
		break;
	case LogLevel::Warn:
		impl->curlvl = boost::log::trivial::severity_level::warning;
		break;
	case LogLevel::Error:
		impl->curlvl = boost::log::trivial::severity_level::error;
		break;
	}

	if (oldlvl != impl->curlvl)
	{
		if (impl->currec)
		{
			impl->slg.push_record(boost::move(impl->currec));
			impl->currec= impl->slg.open_record((keywords::severity = impl->curlvl));
			impl->curstrm.attach_record(impl->currec);
		}
	}

	return *this;
}

Logger& Logger::endl(Logger& log)
{
	std::lock_guard<rsdn::CriticalSection> lock(log.impl->locker);
	if (log.impl->currec)
	{
		log.impl->slg.push_record(boost::move(log.impl->currec));
		log.impl->currec = log.impl->slg.open_record((keywords::severity = log.impl->curlvl));
		log.impl->curstrm.attach_record(log.impl->currec);
	}
	return log;
}

Logger& Logger::operator << (Logger& (__cdecl *_Pfn)(Logger&))
{
	std::lock_guard<rsdn::CriticalSection> lock(impl->locker);
	return ((*_Pfn)(*this));
}

Logger& Logger::operator << (char c)
{
	std::lock_guard<rsdn::CriticalSection> lock(impl->locker);
	if (!impl->currec)
	{
		impl->currec = impl->slg.open_record((keywords::severity = impl->curlvl));
		impl->curstrm.attach_record(impl->currec);
	}
	impl->curstrm << c;
	impl->curstrm.flush();
	return *this;
}

Logger& Logger::operator << (const char* p)
{
	std::lock_guard<rsdn::CriticalSection> lock(impl->locker);
	if (!impl->currec)
	{
		impl->currec = impl->slg.open_record((keywords::severity = impl->curlvl));
		impl->curstrm.attach_record(impl->currec);
	}
	impl->curstrm << p;
	impl->curstrm.flush();
	return *this;
}

Logger& Logger::operator << (const std::string& p)
{
	std::lock_guard<rsdn::CriticalSection> lock(impl->locker);
	if (!impl->currec)
	{
		impl->currec = impl->slg.open_record((keywords::severity = impl->curlvl));
		impl->curstrm.attach_record(impl->currec);
	}
	impl->curstrm << p;
	impl->curstrm.flush();
	return *this;
}

Logger& Logger::operator << (wchar_t c)
{
	std::lock_guard<rsdn::CriticalSection> lock(impl->locker);
	if (!impl->currec)
	{
		impl->currec = impl->slg.open_record((keywords::severity = impl->curlvl));
		impl->curstrm.attach_record(impl->currec);
	}
	impl->curstrm << c;
	impl->curstrm.flush();
	return *this;
}

Logger& Logger::operator << (const wchar_t* p)
{
	std::lock_guard<rsdn::CriticalSection> lock(impl->locker);
	if (!impl->currec)
	{
		impl->currec = impl->slg.open_record((keywords::severity = impl->curlvl));
		impl->curstrm.attach_record(impl->currec);
	}
	impl->curstrm << p;
	impl->curstrm.flush();
	return *this;
}

Logger& Logger::operator << (const std::wstring& p)
{
	std::lock_guard<rsdn::CriticalSection> lock(impl->locker);
	if (!impl->currec)
	{
		impl->currec = impl->slg.open_record((keywords::severity = impl->curlvl));
		impl->curstrm.attach_record(impl->currec);
	}
	impl->curstrm << p;
	impl->curstrm.flush();
	return *this;
}

Logger& Logger::operator << (short val)
{
	std::lock_guard<rsdn::CriticalSection> lock(impl->locker);
	if (!impl->currec)
	{
		impl->currec = impl->slg.open_record((keywords::severity = impl->curlvl));
		impl->curstrm.attach_record(impl->currec);
	}
	impl->curstrm << val;
	impl->curstrm.flush();
	return *this;
}

Logger& Logger::operator << (unsigned short val)
{
	std::lock_guard<rsdn::CriticalSection> lock(impl->locker);
	if (!impl->currec)
	{
		impl->currec = impl->slg.open_record((keywords::severity = impl->curlvl));
		impl->curstrm.attach_record(impl->currec);
	}
	impl->curstrm << val;
	impl->curstrm.flush();
	return *this;
}

Logger& Logger::operator << (int val)
{
	std::lock_guard<rsdn::CriticalSection> lock(impl->locker);
	if (!impl->currec)
	{
		impl->currec = impl->slg.open_record((keywords::severity = impl->curlvl));
		impl->curstrm.attach_record(impl->currec);
	}
	impl->curstrm << val;
	impl->curstrm.flush();
	return *this;
}

Logger& Logger::operator << (unsigned int val)
{
	std::lock_guard<rsdn::CriticalSection> lock(impl->locker);
	if (!impl->currec)
	{
		impl->currec = impl->slg.open_record((keywords::severity = impl->curlvl));
		impl->curstrm.attach_record(impl->currec);
	}
	impl->curstrm << val;
	impl->curstrm.flush();
	return *this;
}

Logger& Logger::operator << (long long val)
{
	std::lock_guard<rsdn::CriticalSection> lock(impl->locker);
	if (!impl->currec)
	{
		impl->currec = impl->slg.open_record((keywords::severity = impl->curlvl));
		impl->curstrm.attach_record(impl->currec);
	}
	impl->curstrm << val;
	impl->curstrm.flush();
	return *this;
}

Logger& Logger::operator << (unsigned long long val)
{
	std::lock_guard<rsdn::CriticalSection> lock(impl->locker);
	if (!impl->currec)
	{
		impl->currec = impl->slg.open_record((keywords::severity = impl->curlvl));
		impl->curstrm.attach_record(impl->currec);
	}
	impl->curstrm << val;
	impl->curstrm.flush();
	return *this;
}

Logger& Logger::operator << (float val)
{
	std::lock_guard<rsdn::CriticalSection> lock(impl->locker);
	if (!impl->currec)
	{
		impl->currec = impl->slg.open_record((keywords::severity = impl->curlvl));
		impl->curstrm.attach_record(impl->currec);
	}
	impl->curstrm << val;
	impl->curstrm.flush();
	return *this;
}

Logger& Logger::operator << (double val)
{
	std::lock_guard<rsdn::CriticalSection> lock(impl->locker);
	if (!impl->currec)
	{
		impl->currec = impl->slg.open_record((keywords::severity = impl->curlvl));
		impl->curstrm.attach_record(impl->currec);
	}
	impl->curstrm << val;
	impl->curstrm.flush();
	return *this;
}

Logger& Logger::operator << (long double val)
{
	std::lock_guard<rsdn::CriticalSection> lock(impl->locker);
	if (!impl->currec)
	{
		impl->currec = impl->slg.open_record((keywords::severity = impl->curlvl));
		impl->curstrm.attach_record(impl->currec);
	}
	impl->curstrm << val;
	impl->curstrm.flush();
	return *this;
}

Logger& Logger::operator << (const void* val)
{
	std::lock_guard<rsdn::CriticalSection> lock(impl->locker);
	if (!impl->currec)
	{
		impl->currec = impl->slg.open_record((keywords::severity = impl->curlvl));
		impl->curstrm.attach_record(impl->currec);
	}
	impl->curstrm << val;
	impl->curstrm.flush();
	return *this;
}

#pragma endregion

