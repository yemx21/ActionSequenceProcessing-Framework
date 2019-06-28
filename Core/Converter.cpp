#include "Converter.h"
#include <Windows.h>
#include <exception>
#include <vector>
using namespace rsdn;

std::wstring Converter::Convert(const std::string & str)
{
	std::wstring returnValue;
	auto wideCharSize = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, str.c_str(), -1, nullptr, 0);
	if (wideCharSize == 0)
	{
		return returnValue;
	}
	returnValue.resize(wideCharSize);
	wideCharSize = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, str.c_str(), -1, &returnValue[0], wideCharSize);
	if (wideCharSize == 0)
	{
		returnValue.resize(0);
		return returnValue;
	}
	returnValue.resize(wideCharSize - 1);
	return returnValue;
}

std::string Converter::Convert(const std::wstring & str)
{
	std::string returnValue;
	auto narrowCharSize = WideCharToMultiByte(CP_ACP, MB_PRECOMPOSED, str.c_str(), -1, nullptr, 0, 0, 0);
	if (narrowCharSize == 0)
	{
		return returnValue;
	}
	returnValue.resize(narrowCharSize);
	narrowCharSize = WideCharToMultiByte(CP_ACP, MB_PRECOMPOSED, str.c_str(), -1, &returnValue[0], narrowCharSize, 0, 0);
	if (narrowCharSize == 0)
	{
		returnValue.resize(0);
		return returnValue;
	}
	returnValue.resize(narrowCharSize - 1);
	return returnValue;
}