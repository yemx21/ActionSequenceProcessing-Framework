#include "FileStreamReader.h"
#include "Encodings.h"

int FileStreamReader::ReadBuffer()
{
	charLen = 0;
	charPos = 0;
	if (!_checkPreamble)
	{
		byteLen = 0;
	}
	do
	{
		if (_checkPreamble)
		{
			int num = base->Read(byteBuffer, byteBufferSize, bytePos, byteBufferSize - bytePos);

			if (num == 0)
			{
				if (byteLen > 0)
				{
					charLen += EncodingHelper::GetUCS(byteBuffer, byteBufferSize, 0, byteLen, charBuffer, _maxCharsPerBuffer, charLen, _encoding, _loc, _cloc);
					byteLen = 0;
					bytePos = 0;
				}

				return charLen;
			}

			byteLen += num;
		}
		else
		{
			byteLen = base->Read(byteBuffer, byteBufferSize, 0, byteBufferSize);
			if (byteLen == 0)
			{
				return charLen;
			}
		}
		_isBlocked = (unsigned __int64)byteLen < byteBufferSize;

		if (IsPreamble()) continue;

		charLen += EncodingHelper::GetUCS(byteBuffer, byteBufferSize, 0, byteLen, charBuffer, _maxCharsPerBuffer, charLen, _encoding, _loc, _cloc);
	} while (charLen == 0);
	return charLen;
}

FileStreamReader::~FileStreamReader()
{
	if(_cloc) _free_locale(_cloc);
	SafeDelete(byteBuffer);
	if (base) { base->Close(); delete base; base = nullptr; }
}

bool FileStreamReader::IsPreamble()
{
	if (!_checkPreamble)
		return _checkPreamble;

	int len = (byteLen >= (_preamble.size())) ? (_preamble.size() - bytePos) : (byteLen - bytePos);

	for (int i = 0; i<len; i++, bytePos++) 
	{
		if (byteBuffer[bytePos] != _preamble[bytePos]) 
		{
			bytePos = 0;
			_checkPreamble = false;
			break;
		}
	}

	if (_checkPreamble) 
	{
		if (bytePos == _preamble.size())
		{
			memcpy(byteBuffer + _preamble.size(), byteBuffer, byteLen - _preamble.size());
			byteLen -= _preamble.size();
			bytePos = 0;
			_checkPreamble = false;
		}
	}

	return _checkPreamble;
}

FileStreamReader::FileStreamReader(const wchar_t* path, Encodings encoding, const std::locale& loc, bool nothrow, bool detectEncodingFromByteOrderMarks, int bufferSize)
{
	_cloc = nullptr;
	if (bufferSize < 128) bufferSize = 128;
	FileStream* fileStream = new FileStream();
	try
	{
		if (fileStream->Open(path, FileAccess::Read, FileShare::Read, FileMode::Open, bufferSize, nothrow))
		{
			_loc = loc;
			_cloc = _create_locale(LC_ALL, _loc.name().c_str());

			byteBufferSize = bufferSize;
			base = fileStream;

			byteBuffer = new char[bufferSize];

			if (detectEncodingFromByteOrderMarks)
			{
				char szFlag[4] = { 0 };
				fileStream->Read(szFlag, sizeof(char) * 4, 0, sizeof(char) * 4, false);
				if ((unsigned char)szFlag[0] == 0xFF && (unsigned char)szFlag[1] == 0xFE)
				{
					_encoding = Encodings::Utf16LE;
					fileStream->Seek(sizeof(char) * 2, SeekOrigin::Begin);
				}
				else 	if ((unsigned char)szFlag[0] == 0xFF && (unsigned char)szFlag[1] == 0xFF)
				{
					_encoding = Encodings::Utf16BE;
					fileStream->Seek(sizeof(char) * 2, SeekOrigin::Begin);
				}
				else if ((unsigned char)szFlag[0] == 0xEF && (unsigned char)szFlag[1] == 0xBB && (unsigned char)szFlag[2] == 0xBF)
				{
					_encoding = Encodings::Utf8;
					fileStream->Seek(sizeof(char) * 3, SeekOrigin::Begin);
				}
				else 	if ((unsigned char)szFlag[0] == 0xFF && (unsigned char)szFlag[1] == 0xFE && (unsigned char)szFlag[2] == 0x00 && (unsigned char)szFlag[3] == 0x00)
				{
					_encoding = Encodings::Utf32LE;
					fileStream->Seek(sizeof(char) * 4, SeekOrigin::Begin);
				}
				else 	if ((unsigned char)szFlag[0] == 0x00 && (unsigned char)szFlag[1] == 0x00 && (unsigned char)szFlag[2] == 0xFE && (unsigned char)szFlag[3] == 0xFF)
				{
					_encoding = Encodings::Utf32BE;
					fileStream->Seek(sizeof(char) * 4, SeekOrigin::Begin);
				}
				else
				{
					_encoding = Encodings::Ansi;
					fileStream->Seek(0, SeekOrigin::Begin);
				}
			}
			else
				_encoding = encoding;

			_maxCharsPerBuffer = EncodingHelper::GetMaxCharCount(bufferSize, _encoding);
			charBuffer = new wchar_t[_maxCharsPerBuffer]{0};
			byteLen = 0;
			bytePos = 0;
			charPos = 0;
			charLen = 0;
			_preamble = EncodingHelper::GetPreamble(_encoding);
			_checkPreamble = _encoding != Encodings::Ansi;
			_isBlocked = false;
			_closable = true;
		}
	}
	catch (...)
	{
		if (!nothrow) throw;
	}
}

int FileStreamReader::Peek(bool nothrow) 
{
	if (base == nullptr)
	{
		if (!nothrow) throw std::exception("file is not opened");
		return -1;
	}
	if (charPos == charLen && (_isBlocked || ReadBuffer() == 0))
	{
		return -1;
	}
	return charBuffer[charPos];
}

bool FileStreamReader::IsEndOfStream() 
{
	if (charPos < charLen)  return false;
	int num = ReadBuffer();
	return num == 0;
}

int FileStreamReader::Read(bool nothrow)
{
	if (base == nullptr)
	{
		if (!nothrow) throw std::exception("file is not opened");
		return -1;
	}
	if (charPos == charLen && ReadBuffer() == 0)
	{
		return -1;
	}
	int num = charBuffer[charPos];
	charPos++;
	return num;
}

unsigned __int64 FileStreamReader::ReadBuffer(wchar_t* userBuffer, int userOffset, int desiredChars, bool& readToUserBuffer)
{
	charLen = 0;
	charPos = 0;
	if (!_checkPreamble)
	{
		byteLen = 0;
	}
	int chars = 0;
	readToUserBuffer = desiredChars >= _maxCharsPerBuffer;
	do
	{
		if (!_checkPreamble)
		{
			byteLen = base->Read(byteBuffer, byteBufferSize, 0, byteBufferSize);
			if (byteLen == 0)
			{
				break;
			}
		}
		else
		{
			int num = base->Read(byteBuffer, byteBufferSize, bytePos, byteBufferSize - bytePos);
			if (num == 0)
			{
				if (byteLen > 0)
				{
					if (!readToUserBuffer)
					{
						chars = EncodingHelper::GetUCS(byteBuffer, byteBufferSize, 0, byteLen, charBuffer, _maxCharsPerBuffer, chars, _encoding, _loc, _cloc);
						charLen += chars;
					}
					else
					{
						chars = EncodingHelper::GetUCS(byteBuffer, byteBufferSize, 0, byteLen, userBuffer, userOffset + chars, chars, _encoding, _loc, _cloc);
						charLen = 0;
					}
				}
				return chars;
			}
			byteLen += num;
		}
		_isBlocked = (unsigned __int64)byteLen < byteBufferSize;
		if (IsPreamble())
		{
			continue;
		}
		charPos = 0;
		if (!readToUserBuffer)
		{
			chars = EncodingHelper::GetUCS(byteBuffer, byteBufferSize, 0, byteLen, charBuffer, _maxCharsPerBuffer, chars, _encoding, _loc, _cloc);
			charLen += chars;
		}
		else
		{
			chars = chars + EncodingHelper::GetUCS(byteBuffer, byteBufferSize, 0, byteLen, userBuffer, userOffset + chars, chars, _encoding, _loc, _cloc);
			charLen = 0;
		}
	} while (chars == 0);
	_isBlocked = _isBlocked & (chars < desiredChars);
	return chars;
}

unsigned __int64 FileStreamReader::Read(wchar_t* buffer, unsigned __int64 bufferCount, unsigned __int64 index, unsigned __int64 count, bool nothrow) 
{
	if (buffer == nullptr)
	{
		if (!nothrow) throw std::exception("empty buffer");
		return 0UL;
	}
	if (bufferCount - index < count)
	{
		if (!nothrow) throw std::exception("invalid index and count");
		return 0UL;
	}

	if (base == nullptr)
	{
		if (!nothrow) throw std::exception("file is not opened");
		return 0UL;
	}

	int num = 0;
	bool flag = false;
	do
	{
		if (count == 0)
		{
			break;
		}
		int num1 = charLen - charPos;
		if (num1 == 0)
		{
			num1 = ReadBuffer(buffer, index + num, count, flag);
		}
		if (num1 == 0)
		{
			break;
		}
		if (num1 > count)
		{
			num1 = count;
		}
		if (!flag)
		{
			memcpy(buffer + (index + num) * 2, charBuffer + charPos * 2, num1 * 2);
			charPos += num1;
		}
		num = num + num1;
		count = count - num1;
	} while (!_isBlocked);
	return num;
}


unsigned __int64 FileStreamReader::ReadBlock(wchar_t* buffer, unsigned __int64 bufferCount, unsigned __int64 index, unsigned __int64 count, bool nothrow) 
{
	if (buffer == nullptr)
	{
		if (!nothrow) throw std::exception("empty buffer");
		return 0UL;
	}
	if (bufferCount - index < count)
	{
		if (!nothrow) throw std::exception("invalid index and count");
		return 0UL;
	}

	if (base == nullptr)
	{
		if (!nothrow) throw std::exception("file is not opened");
		return 0UL;
	}
	return FileReader::ReadBlock(buffer, bufferCount, index, count, nothrow);
}

std::wstring FileStreamReader::ReadLine(bool nothrow)
{
	std::wstring stringBuilder{};
	if (base == nullptr)
	{
		if (!nothrow) throw std::exception("file is not opened");
		return stringBuilder;
	}

	if (charPos == charLen)
	{
		if (ReadBuffer() == 0) return L"";
	}

	do
	{
		int num = charPos;
		do
		{
			wchar_t chr = charBuffer[num];
			if (chr == '\r' || chr == '\n')
			{
				if (stringBuilder.empty())
				{
					stringBuilder = std::wstring(charBuffer + charPos, num - charPos);
				}
				else
				{
					stringBuilder.append((charBuffer + charPos), num - charPos);
				}
				charPos = num + 1;
				if (chr == '\r' && (charPos < charLen || ReadBuffer() > 0) && charBuffer[charPos] == '\n')
				{
					charPos++;
				}
				return stringBuilder;
			}
			num++;
		} while (num < charLen);
		num = charLen - charPos;
		if (stringBuilder.empty())
		{
			stringBuilder.reserve(num + 80);
		}
		stringBuilder.append((wchar_t*)(charBuffer + charPos), num);
	} while (ReadBuffer() > 0);
	return stringBuilder;
}

std::wstring FileStreamReader::ReadToEnd(bool nothrow)
{
	std::wstring stringBuilder{};
	stringBuilder.reserve(charLen - charPos);
	do
	{
		stringBuilder.append((wchar_t*)charBuffer + charPos, charLen - charPos);
		charPos = charLen;
		ReadBuffer();
	} while (charLen > 0);
	return stringBuilder;
}