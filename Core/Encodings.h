#pragma once
#include "FileReader.h"

class EncodingHelper
{
public:
	static std::vector<char> GetPreamble(Encodings encode);

	static unsigned __int64 GetMaxCharCount(unsigned int byteCount, Encodings encoding);

	static unsigned __int64 Utf8GetUCS(char* bytes, unsigned __int64 byteCount, wchar_t* chars, unsigned __int64 charCount, const std::locale& loc, bool nothrow = true);

	static unsigned __int64 Utf16LEGetUCS(char* bytes, unsigned __int64 byteCount, wchar_t* chars, unsigned __int64 charCount, const std::locale& loc, bool nothrow = true);

	static unsigned __int64 Utf16BEGetUCS(char* bytes, unsigned __int64 byteCount, wchar_t* chars, unsigned __int64 charCount, const std::locale& loc, bool nothrow = true);

	static unsigned __int64 Utf32LEGetUCS(char* bytes, unsigned __int64 byteCount, wchar_t* chars, unsigned __int64 charCount, const std::locale& loc, bool nothrow = true);

	static unsigned __int64 Utf32BEGetUCS(char* bytes, unsigned __int64 byteCount, wchar_t* chars, unsigned __int64 charCount, const std::locale& loc, bool nothrow = true);

	static unsigned __int64 GetUCS(char* bytes, unsigned __int64 byteSize, unsigned __int64 byteIndex, unsigned int byteCount, wchar_t* chars, unsigned __int64 charSize, unsigned __int64 charIndex, Encodings encoding, const std::locale& loc, _locale_t cloc, bool nothrow = true);

};