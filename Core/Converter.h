#pragma once
#include <string>
#include "Core_Config.h"
namespace rsdn
{
	class CORE_API Converter
	{
	public:
		static std::wstring Convert(const std::string & str);
		static std::string Convert(const std::wstring & str);
	};
}