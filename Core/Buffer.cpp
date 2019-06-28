#include "Buffer.h"
#include "Buffer_impl.h"
#include "FileStream.h"
#include <exception>
using namespace rsdn;

bool IBuffer::Save(const std::wstring& path)
{
	return false;
}

bool IBuffer::Load(const std::wstring& path)
{
	return false;
}

#pragma region BufferFileReader

namespace rsdn
{
	class BufferFileReader;
	typedef std::shared_ptr<BufferFileReader> BufferFileReaderPtr;

	class BufferFileReader
	{
	protected:
		friend rsdn::Buffer;
		friend rsdn::BasicBuffer;
		friend rsdn::BinaryBuffer;
		FileStream reader;
		int version;
		int basetype;
		int buffertype;
	public:
		static BufferFileReaderPtr Create(const std::wstring& path)
		{
			try
			{
				BufferFileReaderPtr ret = std::make_shared<BufferFileReader>();
				if (ret->reader.Open(path.c_str(), FileAccess::Read, FileShare::Read, FileMode::Open, 4096, false))
				{
					char flagA[4];
					char flagB[4];
					ret->version = 0;
					ret->basetype = 0;
					ret->buffertype = 0;
					ret->reader.Read(flagA, sizeof(char) * 4, 0, sizeof(char) * 4, false);
					if (flagA[0] == 'R' && flagA[1] == 'S' && flagA[2] == 'D' && flagA[3] == 'N')
					{
						if (flagB[0] == 'B' && flagB[1] == 'U' && flagB[2] == 'F' && flagB[3] == 'F')
						{
							ret->reader.Read<int>(ret->version, false);
							ret->reader.Read<int>(ret->basetype, false);
							ret->reader.Read<int>(ret->buffertype, false);
							return ret;
						}
					}
				}
				return nullptr;
			}
			catch (...)
			{

			}
			return nullptr;
		}
	};
}


#pragma endregion

#pragma region Buffer
Buffer::Buffer()
{
	impl = std::make_shared<Buffer_impl>();
}

Buffer::~Buffer()
{
	
}

Buffer::Buffer(const int a)
{
	std::vector<int> shape;
	shape.push_back(a);

	impl = std::make_shared<Buffer_impl>(shape);
}

Buffer::Buffer(const int a, const int b)
{
	std::vector<int> shape;
	shape.push_back(a);
	shape.push_back(b);

	impl = std::make_shared<Buffer_impl>(shape);
}

Buffer::Buffer(const int a, const int b, const int c)
{
	std::vector<int> shape;
	shape.push_back(a);
	shape.push_back(b);
	shape.push_back(c);

	impl = std::make_shared<Buffer_impl>(shape);
}

Buffer::Buffer(const int a, const int b, const int c, const int d)
{
	impl = std::make_shared<Buffer_impl>(a, b, c, d);
}

Buffer::Buffer(const std::vector<int>& shape)
{
	impl = std::make_shared<Buffer_impl>(shape);
}

const std::vector<int>& Buffer::Shape() const
{
	return impl->shape;
}

bool Buffer::Reshape(const std::vector<int>& count)
{
	try
	{
		impl->Reshape(count);
		return true;
	}
	catch (...)
	{
	}
	return false;
}

bool Buffer::Reshape(const int a)
{
	std::vector<int> shapevec(1);
	shapevec[0] = a;
	return Reshape(shapevec);
}

bool Buffer::Reshape(const int a, const int b)
{
	std::vector<int> shapevec(2);
	shapevec[0] = a;
	shapevec[1] = b;
	return Reshape(shapevec); 
}

bool Buffer::Reshape(const int a, const int b, const int c)
{
	std::vector<int> shapevec(3);
	shapevec[0] = a;
	shapevec[1] = b;
	shapevec[2] = c;
	return Reshape(shapevec);
}

bool Buffer::Reshape(const int a, const int b, const int c, const int d)
{
	std::vector<int> shapevec(4);
	shapevec[0] = a;
	shapevec[1] = b;
	shapevec[2] = c;
	shapevec[3] = d;
	return Reshape(shapevec);
}

bool Buffer::ReshapeLike(BufferPtr buf)
{
	try
	{
		impl->ReshapeLike(*buf->impl);
		return true;
	}
	catch (...)
	{
	}
	return false;
}

bool Buffer::IsShapeSameAs(BufferPtr buf)
{
	try
	{
		return impl->IsShapeSameAs(*buf->impl);
	}
	catch (...)
	{
	}
	return false;
}


datatype* Buffer::GetCpu()
{
	return impl->GetCpu();
}

datatype* Buffer::GetGpu()
{
	return impl->GetGpu();
}

bool Buffer::Save(const std::wstring& path)
{
	

	return false;
}

bool Buffer::Load(const std::wstring& path)
{
	try
	{
		auto file = BufferFileReader::Create(path);
		if (file)
		{
			if (file->buffertype == 1)
			{
				int shapesize = 0;
				file->reader.Read<int>(shapesize, false);
				if (shapesize > 0)
				{
					std::vector<int> shape;
					long long len = 0;
					for (int i = 0; i < shapesize; i++)
					{
						int size = 0;
						file->reader.Read<int>(size, false);
						shape.push_back(size);
						if (i == 0) len = size; else len *= size;
					}
					if (Reshape(shape))
					{
						file->reader.Read((char*)impl->GetCpu(), sizeof(datatype)* len, 0, sizeof(datatype)*len, false);
					}
				}
			}
		}
	}
	catch (...)
	{

	}
	return false;
}

#pragma endregion

#pragma region BasicBuffer
BasicBuffer::BasicBuffer() : data(), diff(), count(0), capacity(0), elementsize(0)
{

}

BasicBuffer::BasicBuffer(const int elesize) : data(), diff(), count(0), capacity(0), elementsize(elesize)
{

}

BasicBuffer::BasicBuffer(const int elesize, const int num, const int channels, const int height, const int width) : capacity(0)
{
	elementsize = 0;
	Reshape(elesize, num, channels, height, width);
}

BasicBuffer::BasicBuffer(const int elesize, const std::vector<int>& shapevec) : capacity(0)
{
	elementsize = 0;
	Reshape(elesize, shapevec);
}

void BasicBuffer::ReshapeLike(const BasicBuffer& other)
{
	Reshape(other.elementsize, other.shape);
}

void BasicBuffer::Reshape(const int elesize, const int num, const int channels, const int height, const int width)
{
	std::vector<int> shapevec(4);
	shapevec[0] = num;
	shapevec[1] = channels;
	shapevec[2] = height;
	shapevec[3] = width;
	Reshape(elesize, shapevec);
}

void BasicBuffer::Reshape(const int elesize, const std::vector<int>& shapevec)
{
	count = 1;
	elementsize = elesize;
	shape.resize(shapevec.size());
	if (!shapedata || shapedata->memsize < shapevec.size() * sizeof(int))
	{
		shapedata.reset(new SwapBuffer(shapevec.size() * sizeof(int)));
	}
	int* shape_data = static_cast<int*>(shapedata->mutablecpu());
	for (int i = 0; i < shapevec.size(); ++i)
	{
		if (shape[i] < 0) throw std::exception("wrong shape size");
		if (count != 0)
		{
			if (shape[i] > INT_MAX / count) throw std::exception("shape size exceeds INT_MAX");
		}
		count *= shapevec[i];
		shape[i] = shapevec[i];
		shape_data[i] = shapevec[i];
	}
	if (count > capacity)
	{
		capacity = count;
		data.reset(new SwapBuffer(capacity * elesize));
		diff.reset(new SwapBuffer(capacity * elesize));
	}
}

void BasicBuffer::Reshape(const int num, const int channels, const int height, const int width)
{
	std::vector<int> shapevec(4);
	shapevec[0] = num;
	shapevec[1] = channels;
	shapevec[2] = height;
	shapevec[3] = width;
	Reshape(shapevec);
}

void BasicBuffer::Reshape(const std::vector<int>& shapevec)
{
	count = 1;
	shape.resize(shapevec.size());
	if (!shapedata || shapedata->memsize < shapevec.size() * sizeof(int))
	{
		shapedata.reset(new SwapBuffer(shapevec.size() * sizeof(int)));
	}
	int* shape_data = static_cast<int*>(shapedata->mutablecpu());
	for (int i = 0; i < shapevec.size(); ++i)
	{
		if (shape[i] < 0) throw std::exception("wrong shape size");
		if (count != 0)
		{
			if (shape[i] > INT_MAX / count) throw std::exception("shape size exceeds INT_MAX");
		}
		count *= shapevec[i];
		shape[i] = shapevec[i];
		shape_data[i] = shapevec[i];
	}
	if (count > capacity)
	{
		capacity = count;
		data.reset(new SwapBuffer(capacity * elementsize));
		diff.reset(new SwapBuffer(capacity * elementsize));
	}
}

const std::vector<int>& BasicBuffer::GetShape() const
{
	return shape;
}

bool BasicBuffer::IsShapeSameAs(const BasicBuffer& other)
{
	if (shape.size() != other.shape.size()) return false;
	for (size_t i = 0; i < shape.size(); i++)
	{
		if (shape[i] != other.shape[i]) return false;
	}
	return true;
}

void* BasicBuffer::GetCpu()
{
	return data ? data->mutablecpu() : nullptr;
}

void* BasicBuffer::GetGpu()
{
	return data ? data->mutablegpu() : nullptr;
}

bool BasicBuffer::Save(const std::wstring& path)
{


	return false;
}

__forceinline int BasicBufferGetBaseTypeSize(int ty)
{
	switch (ty)
	{
	case 1:
		return sizeof(bool);
	case 2:
		return sizeof(int8_t);
	case 3:
		return sizeof(uint8_t);
	case 4:
		return sizeof(int16_t);
	case 5:
		return sizeof(uint16_t);
	case 6:
		return sizeof(int32_t);
	case 7:
		return sizeof(uint32_t);
	case 8:
		return sizeof(int64_t);
	case 9:
		return sizeof(uint64_t);
	case 10:
		return sizeof(float);
	case 11:
		return sizeof(double);
	default:
		break;
	}
	return 0;
}

bool BasicBuffer::Load(const std::wstring& path)
{
	try
	{
		auto file = BufferFileReader::Create(path);
		if (file)
		{
			if (file->buffertype == 2)
			{
				int typememsize = BasicBufferGetBaseTypeSize(file->basetype);
				if (typememsize != 0)
				{
					int shapesize = 0;
					file->reader.Read<int>(shapesize, false);
					if (shapesize > 0)
					{
						std::vector<int> shape;
						long long len = 0;
						for (int i = 0; i < shapesize; i++)
						{
							int size = 0;
							file->reader.Read<int>(size, false);
							shape.push_back(size);
							if (i == 0) len = size; else len *= size;
						}
						Reshape(typememsize, shape);
						file->reader.Read((char*)GetCpu(), typememsize * len, 0, typememsize * len, false);
					}
				}
			}
		}
	}
	catch (...)
	{

	}
	return false;
}

IBufferPtr BasicBuffer::LoadAsGenericBuffer(const std::wstring& path)
{
	try
	{
		auto file = BufferFileReader::Create(path);
		if (file)
		{
			if (file->buffertype == 2)
			{
				int typememsize = BasicBufferGetBaseTypeSize(file->basetype);
				if (typememsize != 0)
				{
					int shapesize = 0;
					file->reader.Read<int>(shapesize, false);
					if (shapesize > 0)
					{
						std::vector<int> shape;
						long long len = 0;
						for (int i = 0; i < shapesize; i++)
						{
							int size = 0;
							file->reader.Read<int>(size, false);
							shape.push_back(size);
							if (i == 0) len = size; else len *= size;
						}

						switch (file->basetype)
						{
						case 1:
						{
							std::shared_ptr<GenericBuffer<bool>> buf = std::make_shared<GenericBuffer<bool>>();
							buf->Reshape(shape);
							file->reader.Read((char*)buf->GetCpu(), typememsize * len, 0, typememsize * len, false);
							return buf;
						}
						case 2:
						{
							std::shared_ptr<GenericBuffer<int8_t>> buf = std::make_shared<GenericBuffer<int8_t>>();
							buf->Reshape(shape);
							file->reader.Read((char*)buf->GetCpu(), typememsize * len, 0, typememsize * len, false);
							return buf;
						}
						case 3:
						{
							std::shared_ptr<GenericBuffer<uint8_t>> buf = std::make_shared<GenericBuffer<uint8_t>>();
							buf->Reshape(shape);
							file->reader.Read((char*)buf->GetCpu(), typememsize * len, 0, typememsize * len, false);
							return buf;
						}
						case 4:
						{
							std::shared_ptr<GenericBuffer<int16_t>> buf = std::make_shared<GenericBuffer<int16_t>>();
							buf->Reshape(shape);
							file->reader.Read((char*)buf->GetCpu(), typememsize * len, 0, typememsize * len, false);
							return buf;
						}
						case 5:
						{
							std::shared_ptr<GenericBuffer<uint16_t>> buf = std::make_shared<GenericBuffer<uint16_t>>();
							buf->Reshape(shape);
							file->reader.Read((char*)buf->GetCpu(), typememsize * len, 0, typememsize * len, false);
							return buf;
						}
						case 6:
						{
							std::shared_ptr<GenericBuffer<int32_t>> buf = std::make_shared<GenericBuffer<int32_t>>();
							buf->Reshape(shape);
							file->reader.Read((char*)buf->GetCpu(), typememsize * len, 0, typememsize * len, false);
							return buf;
						}
						case 7:
						{
							std::shared_ptr<GenericBuffer<uint32_t>> buf = std::make_shared<GenericBuffer<uint32_t>>();
							buf->Reshape(shape);
							file->reader.Read((char*)buf->GetCpu(), typememsize * len, 0, typememsize * len, false);
							return buf;
						}
						case 8:
						{
							std::shared_ptr<GenericBuffer<int64_t>> buf = std::make_shared<GenericBuffer<int64_t>>();
							buf->Reshape(shape);
							file->reader.Read((char*)buf->GetCpu(), typememsize * len, 0, typememsize * len, false);
							return buf;
						}
						case 9:
						{
							std::shared_ptr<GenericBuffer<uint64_t>> buf = std::make_shared<GenericBuffer<uint64_t>>();
							buf->Reshape(shape);
							file->reader.Read((char*)buf->GetCpu(), typememsize * len, 0, typememsize * len, false);
							return buf;
						}
						case 10:
						{
							std::shared_ptr<GenericBuffer<float>> buf = std::make_shared<GenericBuffer<float>>();
							buf->Reshape(shape);
							file->reader.Read((char*)buf->GetCpu(), typememsize * len, 0, typememsize * len, false);
							return buf;
						}
						case 11:
						{
							std::shared_ptr<GenericBuffer<double>> buf = std::make_shared<GenericBuffer<double>>();
							buf->Reshape(shape);
							file->reader.Read((char*)buf->GetCpu(), typememsize * len, 0, typememsize * len, false);
							return buf;
						}
						default:
							break;
						}
					}
				}
			}
		}
	}
	catch (...)
	{

	}
	return nullptr;
}

#pragma endregion

#pragma region BinaryBuffer
BinaryBuffer::BinaryBuffer()
{
	impl = std::make_shared<BinaryBuffer_impl>();
}

BinaryBuffer::~BinaryBuffer()
{

}

BinaryBuffer::BinaryBuffer(const sizetype shape)
{
	impl = std::make_shared<BinaryBuffer_impl>(shape);
}

const sizetype BinaryBuffer::Shape() const
{
	return impl->shape;
}

bool BinaryBuffer::Reshape(const sizetype count)
{
	try
	{
		impl->Reshape(count);
		return true;
	}
	catch (...)
	{
	}
	return false;
}

bool BinaryBuffer::ReshapeLike(BinaryBufferPtr buf)
{
	try
	{
		impl->ReshapeLike(*buf->impl);
		return true;
	}
	catch (...)
	{
	}
	return false;
}

void* BinaryBuffer::GetCpu()
{
	return impl->GetCpu();
}

void* BinaryBuffer::GetGpu()
{
	return impl->GetGpu();
}

bool BinaryBuffer::Save(const std::wstring& path)
{
	return false;
}

bool BinaryBuffer::Load(const std::wstring& path)
{
	try
	{
		auto file = BufferFileReader::Create(path);
		if (file)
		{
			if (file->buffertype == 3)
			{
				unsigned long long len = 0;
				file->reader.Read<unsigned long long>(len, false);
				if (len > 0)
				{
					if (Reshape(len))
					{
						file->reader.Read((char*)impl->GetCpu(), sizeof(char)* len, 0, sizeof(char)*len, false);
					}
				}
			}
		}
	}
	catch (...)
	{

	}
	return false;
}


CPUReadonlyBufferPtr BinaryBuffer::GetCpuReadonlyBuffer()
{
	return std::make_shared<CPUReadonlyBuffer>(impl->GetCpu(), impl->shape);
}
#pragma endregion

#pragma region CPUReadonlyBuffer

CPUReadonlyBuffer::CPUReadonlyBuffer(const void* mem, size_t size) : address(mem), len(size), cur(0)
{

}

void CPUReadonlyBuffer::ReadCore(void* addr, size_t size)
{
	if (cur + size > len) 
		throw std::exception("out of buffer boundary");
	memcpy(addr, (char*)address + cur, size);
	cur += size;
}

void CPUReadonlyBuffer::ReadBuffer(BufferPtr buf, const int a)
{
	size_t size = a * sizeof(datatype);
	if (cur + size > len) throw std::exception("out of buffer boundary");
	buf->Reshape(a);
	memcpy(buf->GetCpu(), (char*)address + cur, size);
	cur += size;
}

void CPUReadonlyBuffer::ReadBuffer(BufferPtr buf, const int a, const int b)
{
	size_t size = a * b * sizeof(datatype);
	if (cur + size > len) throw std::exception("out of buffer boundary");
	buf->Reshape(a, b);
	memcpy(buf->GetCpu(), (char*)address + cur, size);
	cur += size;
}

void CPUReadonlyBuffer::ReadBuffer(BufferPtr buf, const int a, const int b, const int c)
{
	size_t size = a * b * c * sizeof(datatype);
	if (cur + size > len) throw std::exception("out of buffer boundary");
	buf->Reshape(a, b, c);
	memcpy(buf->GetCpu(), (char*)address + cur, size);
	cur += size;
}

void CPUReadonlyBuffer::ReadBuffer(BufferPtr buf, const int a, const int b, const int c, const int d)
{
	size_t size = a*b*c*d * sizeof(datatype);
	if (cur + size > len) throw std::exception("out of buffer boundary");
	buf->Reshape(a, b, c, d);
	memcpy(buf->GetCpu(), (char*)address + cur, size);
	cur += size;
}

std::wstring CPUReadonlyBuffer::ReadString()
{
	std::wstring ret;
	unsigned int len = 0u;
	Read(&len, 1);
	if(len)
	{
		ret.resize(len);
		Read((char*)ret.data(), len);
	}
	return ret;
}

void CPUReadonlyBuffer::Seek(long long pos, SeekOrigin origin)
{
	size_t nextcur = cur;
	switch (origin)
	{
	case SeekOrigin::Begin:
		nextcur = pos;
		break;
	case SeekOrigin::Current:
		nextcur += pos;
		break;
	case SeekOrigin::End:
		nextcur = (long long)len - 1 + pos;
		break;
	}

	if (nextcur >= len) throw std::exception("out of buffer boundary");

	cur = nextcur;
}

bool CPUReadonlyBuffer::IsEOF() const
{
	return cur >= len;
}

#pragma endregion