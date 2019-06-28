#include "BSDB.h"
using namespace rsdn;
using namespace rsdn::data;

#pragma region BSDBReader

BSDBReader::BSDBReader():dataindex(-1), version(-1)
{

}

BSDBReader::~BSDBReader()
{
	Close();
}

bool BSDBReader::Load(const std::wstring& path, std::function<bool(const std::wstring&, int)> headcheck)
{
	try
	{
		Close();
		if (!file.Open(path.c_str(), FileAccess::Read, FileShare::Read, FileMode::Open)) return false;

		/*read header*/
		file.ReadString(headerchunk, false);
		file.Read<int>(version, false);

		if (headcheck)
		{
			if (!headcheck(headerchunk, version))
			{
				headerchunk.clear();
				version = -1;
				file.Close();
				return false;
			}
		}

		/*read items*/
		unsigned int itemcount = 0u;
		file.Read<unsigned int>(itemcount, false);
		if (itemcount)
		{
			for (unsigned int i = 0; i < itemcount; i++)
			{
				std::wstring keystr;
				file.ReadString(keystr, false);
				unsigned long long keyvaluesize = 0ull;
				file.Read<unsigned long long>(keyvaluesize, false);
				if (keyvaluesize)
				{
					BinaryBufferPtr keyvalue = std::make_shared<BinaryBuffer>(keyvaluesize);
					file.Read((char*)keyvalue->GetCpu(), keyvaluesize, 0, keyvaluesize, false);
					items.insert(std::make_pair(keystr, keyvalue));
				}
			}
		}

		/*read chunk table*/
		unsigned long long chunkcount = 0u;
		file.Read<unsigned long long>(chunkcount, false);
		if (chunkcount)
		{
			datatable.resize(chunkcount);
			file.Read((char*)datatable.data(), sizeof(long long)* chunkcount, 0, sizeof(long long)* chunkcount, false);
		}

		return true;
	}
	catch (...) {}

	return false;
}

bool BSDBReader::IsOpen() const
{
	return file.IsOpen();
}

bool BSDBReader::Close()
{
	if (file.IsOpen()) file.Close();
	items.clear();
	datatable.clear();
	headerchunk.clear();
	version = -1;
	dataindex = -1;
	return true;
}

bool BSDBReader::Read(BufferPtr data, sizetype index, std::function<bool(BufferPtr, FileStream&, unsigned long long)> feed)
{
	try
	{
		if (index > (__int64)datatable.size() - 1) return false;
		file.Seek(datatable[index], SeekOrigin::Begin, false);
		unsigned long long len = 0ull;
		file.Read<unsigned long long>(len, false);
		if (!feed(data, file, len)) return false;
		dataindex = index;
		return true;
	}
	catch (...) {}
	return false;
}

bool BSDBReader::Next(BufferPtr data, std::function<bool(BufferPtr, FileStream&, unsigned long long)> feed)
{
	try
	{
		if (dataindex > (__int64)datatable.size() - 1) return false;
		dataindex++;
		unsigned long long len = 0ull;
		file.Read<unsigned long long>(len, false);
		file.Seek(datatable[dataindex], SeekOrigin::Begin, false);
		if (!feed(data, file, len)) return false;
		return true;
	}
	catch (...) {}
	return false;
}

bool BSDBReader::Read(BinaryBufferPtr data, sizetype index)
{
	try
	{
		if (index > (__int64)datatable.size() - 1) return false;
		file.Seek(datatable[index], SeekOrigin::Begin, false);
		unsigned long long len = 0ull;
		file.Read<unsigned long long>(len, false);
		data->Reshape(len);
		file.Read((char*)data->GetCpu(), len, 0, len, false);
		dataindex = index;
		return true;
	}
	catch (...) {}
	return false;
}

bool BSDBReader::Next(BinaryBufferPtr data)
{
	try
	{
		if (dataindex >= (__int64)datatable.size() - 1) return false;
		dataindex++;
		file.Seek(datatable[dataindex], SeekOrigin::Begin, false);
		unsigned long long len = 0ull;
		file.Read<unsigned long long>(len, false);
		data->Reshape(len);
		file.Read((char*)data->GetCpu(), len, 0, len, false);
		return true;
	}
	catch (...) {}
	return false;
}

bool BSDBReader::Seek(sizetype index)
{
	try
	{
		if (dataindex >= (__int64)datatable.size() - 1) return false;
		file.Seek(datatable[index], SeekOrigin::Begin, false);
		dataindex = index - 1;
		return true;
	}
	catch (...) {}
	return false;
}

bool BSDBReader::IsEOF()
{
	return dataindex >= (__int64)datatable.size() - 1;
}

sizetype BSDBReader::ChunkCount() const
{
	return (sizetype)datatable.size();
}

BinaryBufferPtr BSDBReader::operator [](const std::wstring& key)
{
	BinaryBufferPtr ret = nullptr;
	auto iter = items.find(key);
	if (iter != items.end()) ret = iter->second;
	return ret;
}

#pragma endregion

#pragma region BSDBWriter

BSDBWriter::BSDBWriter()
{

}

BSDBWriter::~BSDBWriter()
{
	Close();
}

bool BSDBWriter::Open(const std::wstring& path, const std::wstring& flag, int version)
{
	try
	{
		if (file.Open(path.c_str(), FileAccess::Write, FileShare::ReadWrite, FileMode::Create))
		{
			unsigned int flaglen = (unsigned int)(flag.size() * sizeof(wchar_t));
			file.Write((char*)&flaglen, sizeof(unsigned int), 0, sizeof(unsigned int), false);
			file.Write((char*)flag.c_str(), flaglen, 0, flaglen, false);
			file.Write((char*)&version, sizeof(int), 0, sizeof(int), false);
			file.Flush();
			itemcountpos = (sizetype)file.GetPosition();
			unsigned int itemcount = 0;
			file.Write((char*)&itemcount, sizeof(unsigned int), 0, sizeof(unsigned int), false);
			return true;
		}
	}
	catch(...)
	{

	}
	return false;
}

bool BSDBWriter::IsOpen() const
{
	return file.IsOpen();
}

void BSDBWriter::Close()
{
	file.Flush();
	file.Close();
	chunkpos.clear();
	chunkcount.Reset();
	chunkcountpos.Reset();
	itemcount.Reset();
	itemcountpos.Reset();
	reservechunk.Reset();
}

void BSDBWriter::WriteString(const std::wstring& str)
{
	if (!file.IsOpen()) return;
	try
	{
		unsigned int strlen = (unsigned int)str.size() * sizeof(wchar_t);
		file.Write((char*)&strlen, sizeof(unsigned int), 0, sizeof(unsigned int), false);
		file.Write((char*)str.c_str(), strlen, 0, strlen, false);
	}
	catch (...) {}
}

void BSDBWriter::WriteItem(const std::wstring& key, const void* value, size_t size)
{
	try
	{
		if (!file.IsOpen()) return;
		if (value && size)
		{
			WriteString(key);
			usizetype valsize = (usizetype)size;
			file.Write((char*)&valsize, sizeof(usizetype), 0, sizeof(usizetype), false);
			file.Write((char*)value, size, 0, size, false);
			if (!itemcount.IsInit())
				itemcount = 1;
			else
				itemcount = *itemcount + 1;
		}
	}
	catch (...) {}
}

void BSDBWriter::FinishItems()
{
	try
	{
		if (!file.IsOpen()) return;
		if (itemcountpos && itemcount)
		{
			file.Flush();
			sizetype cur = (sizetype)file.GetPosition();
			file.Seek((usizetype)*itemcountpos, SeekOrigin::Begin);
			unsigned int itemcount_real = *itemcount;
			file.Write((char*)&itemcount_real, sizeof(unsigned int), 0, sizeof(unsigned int), false);
			file.Flush();
			file.Seek((sizetype)cur, SeekOrigin::Begin);
		}
		chunkcountpos = (sizetype)file.GetPosition();
	}
	catch (...) {}
}

void BSDBWriter::ReserveChunks(int count)
{
	try
	{
		if (!file.IsOpen()) return;
		file.Seek(sizeof(usizetype) + sizeof(sizetype) * count, SeekOrigin::Current, false);
		reservechunk = count;
	}
	catch (...) {}
}

void BSDBWriter::WriteChunk(const void* value, size_t size)
{
	try
	{
		if (!file.IsOpen()) return;
		if (value)
		{
			file.Flush();
			chunkpos.push_back((sizetype)file.GetPosition());
			usizetype size_real = (usizetype)size;
			file.Write((char*)&size_real, sizeof(usizetype), 0, sizeof(usizetype), false);
			file.Write((char*)value, size, 0, size, false);
			file.Flush();
			if (!chunkcount.IsInit())
				chunkcount = 1;
			else
				chunkcount = *chunkcount + 1;
		}
	}
	catch (...) {}
}

void BSDBWriter::FinishChunks()
{
	try
	{
		if (!file.IsOpen()) return;
		if (chunkcountpos && reservechunk)
		{
			file.Flush();
			auto cur = file.GetPosition();
			file.Seek((unsigned long long)*chunkcountpos, SeekOrigin::Begin, false);
			usizetype chunkcount_real = chunkcount.IsInit() ? (usizetype)*chunkcount : 0u;
			file.Write((char*)&chunkcount_real, sizeof(usizetype), 0, sizeof(usizetype), false);
			for (usizetype i = 0; i< chunkcount_real; i++)
			{
				sizetype chunkpos_real = chunkpos[(int)i];
				file.Write((char*)&chunkpos_real, sizeof(sizetype), 0, sizeof(sizetype), false);
			}
			file.Flush();
			//file.Seek(cur, SeekOrigin::Begin, false);
		}
	}
	catch (...) {}
}

void BSDBWriter::StartChunk()
{
	try
	{
		if (!file.IsOpen()) return;
		{
			file.Flush();
			auto filecur = file.GetPosition();
			chunkpos.push_back((sizetype)filecur);
			curpos = filecur;
			cursize.Reset();
			usizetype size_real = 0;
			file.Write((char*)&size_real, sizeof(usizetype), 0, sizeof(usizetype), false);
			
		}
	}
	catch (...) {}
}

void BSDBWriter::WriteToChunk(const void* itemvalue, size_t itemsize)
{
	try
	{
		if (!file.IsOpen()) return;
		{
			file.Write((char*)itemvalue, itemsize, 0, itemsize, false);
			if (cursize)
				cursize = *cursize + itemsize;
			else
				cursize = itemsize;
		}
	}
	catch (...) {}
}

void BSDBWriter::EndChunk()
{
	try
	{
		if (curpos && cursize)
		{
			file.Flush();
			auto filecur = file.GetPosition();
			file.Seek(*curpos, SeekOrigin::Begin);
			usizetype size_real = *cursize;
			file.Write((char*)&size_real, sizeof(usizetype), 0, sizeof(usizetype), false);
			if (!chunkcount.IsInit())
				chunkcount = 1;
			else
				chunkcount = *chunkcount + 1;
			file.Seek(filecur, SeekOrigin::Begin);
		}
		cursize.Reset();
		curpos.Reset();
	}
	catch (...) 
	{
		cursize.Reset();
		curpos.Reset();
	}
}

#pragma endregion