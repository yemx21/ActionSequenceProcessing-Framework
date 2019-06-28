#pragma once
#include "FileStream.h"
#include "Buffer.h"
#include <memory>
#include <string>
#include <map>
#include <functional>
#include "Optional.h"

namespace rsdn
{
	namespace data
	{
		/*
		BSDB Data Layout

		Header: (%header[string])(%version[int32])
		Items: (%itemcount[uint32]){(%itemkey[string])(%itemvaluememorysize[uint64])(%itemvalue[void*])}
		Table: (%chunkcount[uint64]){(%chunkpos[int64])}
		Data: {(%chunkmemorysize[uint64])(%chunk[void*])}

		*/

		class CORE_API BSDBReader
		{
		private:
			FileStream file;
			std::wstring headerchunk;
			int version;			
			std::map<std::wstring, BinaryBufferPtr> items;
			std::vector<sizetype> datatable;
			sizetype dataindex;
		public:
			BSDBReader();
			~BSDBReader();

			bool Load(const std::wstring& path, std::function<bool(const std::wstring&, int)> headcheck = nullptr);
			bool IsOpen() const;
			bool Close();

			bool Read(BufferPtr data, sizetype index, std::function<bool(BufferPtr, FileStream&, unsigned long long)> feed);
			bool Next(BufferPtr data, std::function<bool(BufferPtr, FileStream&, unsigned long long)> feed);

			bool Read(BinaryBufferPtr data, sizetype index);
			bool Next(BinaryBufferPtr data);

			bool Seek(sizetype index);

			sizetype ChunkCount() const;

			BinaryBufferPtr operator [](const std::wstring& key);

			template<typename T>
			void GetItemValue(const std::wstring& key, T* val)
			{
				BinaryBufferPtr valbuf = operator[](key);
				if (valbuf)
				{
					valbuf->GetCpuReadonlyBuffer()->Read<T>(val, 1);
				}
			}

			template<typename T>
			void GetItemValues(const std::wstring& key, T* val, size_t count)
			{
				BinaryBufferPtr valbuf = operator[](key);
				if (valbuf)
				{
					valbuf->GetCpuReadonlyBuffer()->Read<T>(val, count);
				}
			}

			bool IsEOF();
		};

		typedef std::shared_ptr<BSDBReader> BSDBReaderPtr;


		class CORE_API BSDBWriter
		{
		private:
			FileStream file;
			std::vector<sizetype> chunkpos;
			Optional<usizetype> chunkcount;
			Optional<sizetype> chunkcountpos;
			Optional<unsigned int> itemcount;
			Optional<sizetype> itemcountpos;
			Optional<int> reservechunk;

			Optional<sizetype> curpos;
			Optional<usizetype> cursize;
		public:
			BSDBWriter();
			~BSDBWriter();

			bool Open(const std::wstring& path, const std::wstring& flag, int version);
			bool IsOpen() const;
			void Close();
			void WriteString(const std::wstring& path);

			void WriteItem(const std::wstring& key, const void* itemvalue, size_t itemsize);
			void FinishItems();

			void ReserveChunks(int count);
			void WriteChunk(const void* itemvalue, size_t itemsize);
			void FinishChunks();

			void StartChunk();
			void WriteToChunk(const void* itemvalue, size_t itemsize);
			void EndChunk();
		};

		typedef std::shared_ptr<BSDBWriter> BSDBWriterPtr;
	}
}