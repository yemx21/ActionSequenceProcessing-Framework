#pragma once
#include "FileReader.h"
#include "FileStream.h"
#include <codecvt>
#include <mbstring.h>

class CORE_API FileStreamReader : public FileReader
{
private:		
	static const int DefaultFileStreamBufferSize = 4096;
	static const int MinBufferSize = 128;
	FileStream* base;

	char* byteBuffer;

	unsigned __int64 byteBufferSize;

	wchar_t* charBuffer;

	int charPos;

	int charLen;

	int byteLen;

	int bytePos;

	int _maxCharsPerBuffer;

	bool _checkPreamble;

	bool _isBlocked;

	bool _closable;

	std::vector<char> _preamble;

	Encodings _encoding;

	std::locale _loc;
	_locale_t _cloc;

	int ReadBuffer();

	bool IsPreamble();

public:
	~FileStreamReader();

	FileStreamReader(const wchar_t* path, Encodings encoding, const std::locale& loc, bool nothrow = true, bool detectEncodingFromByteOrderMarks = true, int bufferSize = 4096);
			

	int Peek(bool nothrow = true) override;

	bool IsEndOfStream() override;

	int Read(bool nothrow = true) override;

	unsigned __int64 ReadBuffer(wchar_t* userBuffer, int userOffset, int desiredChars, bool& readToUserBuffer);


	unsigned __int64 Read(wchar_t* buffer, unsigned __int64 bufferCount, unsigned __int64 index, unsigned __int64 count, bool nothrow = true)override;

	unsigned __int64 ReadBlock(wchar_t* buffer, unsigned __int64 bufferCount, unsigned __int64 index, unsigned __int64 count, bool nothrow = true) override;

	std::wstring ReadLine(bool nothrow = true) override;

	std::wstring ReadToEnd(bool nothrow = true) override;

};