#pragma once
#include "Core_Config.h"
#include <string>

enum class Encodings
{
	Ansi,
	Utf8,
	Utf16LE,
	Unicode = Utf16LE,
	Utf16BE,
	Utf32LE,
	Utf32BE,
};

class CORE_API FileReader
{
public:
	virtual int Peek(bool nothrow = true);

	virtual int Read(bool nothrow = true);

	virtual unsigned __int64 Read(wchar_t* buffer, unsigned __int64 bufferCount, unsigned __int64 index, unsigned __int64 count, bool nothrow = true);

	virtual unsigned __int64 ReadBlock(wchar_t* buffer, unsigned __int64 bufferBytes, unsigned __int64 index, unsigned __int64 count, bool nothrow = true);

	virtual std::wstring ReadLine(bool nothrow = true);

	virtual std::wstring ReadToEnd(bool nothrow = true);

	virtual bool IsEndOfStream();

};
