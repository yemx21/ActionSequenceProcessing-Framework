#pragma once
#include "Core_Config.h"
#include <vector>
#include <memory>
namespace rsdn
{
	class Buffer_impl;
	class Buffer;
	class SwapBuffer;
	typedef std::shared_ptr<Buffer> BufferPtr;

	class CORE_API IBuffer
	{
	public:
		virtual bool Save(const std::wstring& path);
		virtual bool Load(const std::wstring& path);
	};
	typedef std::shared_ptr<IBuffer> IBufferPtr;

	class CORE_API Buffer: public IBuffer
	{
	private:
		std::shared_ptr<Buffer_impl> impl;
	public:
		Buffer();
		~Buffer();
		explicit Buffer(const int a);
		explicit Buffer(const int a, const int b);
		explicit Buffer(const int a, const int b, const int c);
		explicit Buffer(const int a, const int b, const int c, const int d);
		explicit Buffer(const std::vector<int>& shape);

		const std::vector<int>& Shape() const;

		bool Reshape(const int a);
		bool Reshape(const int a, const int b);
		bool Reshape(const int a, const int b, const int c);
		bool Reshape(const int a, const int b, const int c, const int d);
		bool Reshape(const std::vector<int>& shape);
		bool ReshapeLike(BufferPtr buf);
		bool IsShapeSameAs(BufferPtr buf);

		inline sizetype Index(const int a)
		{
			const std::vector<int>& shape = Shape();
			if (a < 0 || a >= shape[0]) return -1;
			return  a;
		}

		inline sizetype Index(const int a, const int b)
		{
			const std::vector<int>& shape = Shape();
			if (a < 0 || a >= shape[0]) return -1;
			if (b < 0 || b >= shape[1]) return -1;
			return  a * shape[1] + b;
		}

		inline sizetype Index(const int a, const int b, const int c)
		{
			const std::vector<int>& shape = Shape();
			if (a < 0 || a >= shape[0]) return -1;
			if (b < 0 || b >= shape[1]) return -1;
			if (c < 0 || c >= shape[2]) return -1;
			return  (a * shape[1] + b) * shape[2] + c;
		}

		inline sizetype Index(const int a, const int b, const int c, const int d)
		{
			const std::vector<int>& shape = Shape();
			if (a < 0 || a >= shape[0]) return -1;
			if (b < 0 || b >= shape[1]) return -1;
			if (c < 0 || c >= shape[2]) return -1;
			if (d < 0 || d >= shape[3]) return -1;
			return  ((a * shape[1] + b) * shape[2] + c) * shape[3] + d;
		}

		inline sizetype Index(const std::vector<int>& indices)
		{
			if (indices.empty()) return -1;
			const std::vector<int>& shape = Shape();
			size_t dim = shape.size();
			if (indices.size() != dim) return -1;
			if (indices[0] < 0 || indices[0] >= shape[0]) return -1;
			sizetype index = indices[0];
			for (size_t i = 1; i < dim; i++)
			{
				if (indices[i] < 0 || indices[i] >= shape[i]) return -1;
				index = index*shape[i] + indices[i];
			}
			return index;
		}

		datatype* GetCpu();
		datatype* GetGpu();

		bool Save(const std::wstring& path) override;
		bool Load(const std::wstring& path) override;
	};
	
	class CORE_API BasicBuffer
	{
	private:
		std::shared_ptr<SwapBuffer> data;
		std::shared_ptr<SwapBuffer> diff;
		std::shared_ptr<SwapBuffer> shapedata;
		std::vector<int> shape;
		int count;
		int capacity;
		int elementsize;

	public:
		BasicBuffer();
		explicit BasicBuffer(const int elesize);
		explicit BasicBuffer(const int elesize, const int num, const int channels, const int height, const int width);
		explicit BasicBuffer(const int elesize, const std::vector<int>& shape);

		void Reshape(const int elesize, const int num, const int channels, const int height, const int width);
		void Reshape(const int elesize, const std::vector<int>& shape);

		void Reshape(const int num, const int channels, const int height, const int width);
		void Reshape(const std::vector<int>& shape);

		void ReshapeLike(const BasicBuffer& other);
		bool IsShapeSameAs(const BasicBuffer& other);

		const std::vector<int>& GetShape() const;

		void* GetCpu();
		void* GetGpu();

		bool Save(const std::wstring& path);
		bool Load(const std::wstring& path);

		static IBufferPtr LoadAsGenericBuffer(const std::wstring& path);

		DISABLE_COPY_AND_ASSIGN(BasicBuffer);
	};
	typedef std::shared_ptr<BasicBuffer> BasicBufferPtr;

	template<typename T>
	class GenericBuffer : public IBuffer
	{
	private:
		BasicBufferPtr impl;
	public:
		GenericBuffer()
		{
			impl = std::make_shared<BasicBuffer>((const int)sizeof(T));
		}

		~GenericBuffer()
		{

		}

		explicit GenericBuffer(const int a)
		{
			std::vector<int> shape;
			shape.push_back(a);

			impl = std::make_shared<BasicBuffer>((const int)sizeof(T), shape);
		}

		explicit GenericBuffer(const int a, const int b)
		{
			std::vector<int> shape;
			shape.push_back(a);
			shape.push_back(b);

			impl = std::make_shared<BasicBuffer>((const int)sizeof(T), shape);
		}

		explicit GenericBuffer(const int a, const int b, const int c)
		{
			std::vector<int> shape;
			shape.push_back(a);
			shape.push_back(b);
			shape.push_back(c);

			impl = std::make_shared<BasicBuffer>((const int)sizeof(T), shape);
		}

		explicit GenericBuffer(const int a, const int b, const int c, const int d)
		{
			impl = std::make_shared<BasicBuffer>((const int)sizeof(T), a, b, c, d);
		}

		explicit GenericBuffer(const std::vector<int>& shape)
		{
			impl = std::make_shared<BasicBuffer>((const int)sizeof(T), shape);
		}

		const std::vector<int>& Shape() const
		{
			return impl->GetShape();
		}

		bool Reshape(const int a)
		{
			std::vector<int> shapevec(1);
			shapevec[0] = a;
			return Reshape(shapevec);
		}

		bool Reshape(const int a, const int b)
		{
			std::vector<int> shapevec(2);
			shapevec[0] = a;
			shapevec[1] = b;
			return Reshape(shapevec);
		}

		bool Reshape(const int a, const int b, const int c)
		{
			std::vector<int> shapevec(3);
			shapevec[0] = a;
			shapevec[1] = b;
			shapevec[2] = c;
			return Reshape(shapevec);
		}

		bool Reshape(const int a, const int b, const int c, const int d)
		{
			std::vector<int> shapevec(4);
			shapevec[0] = a;
			shapevec[1] = b;
			shapevec[2] = c;
			shapevec[3] = d;
			return Reshape(shapevec);
		}

		bool Reshape(const std::vector<int>& shape)
		{
			try
			{
				impl->Reshape(shape);
				return true;
			}
			catch (...)
			{
			}
			return false;
		}

		bool ReshapeLike(std::shared_ptr<GenericBuffer> buf)
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

		bool IsShapeSameAs(std::shared_ptr<GenericBuffer> buf)
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

		inline sizetype Index(const int a)
		{
			const std::vector<int>& shape = Shape();
			if (a < 0 || a >= shape[0]) return -1;
			return a;
		}

		inline sizetype Index(const int a, const int b)
		{
			const std::vector<int>& shape = Shape();
			if (a < 0 || a >= shape[0]) return -1;
			if (b < 0 || b >= shape[1]) return -1;
			return a * shape[1] + b;
		}

		inline sizetype Index(const int a, const int b, const int c)
		{
			const std::vector<int>& shape = Shape();
			if (a < 0 || a >= shape[0]) return -1;
			if (b < 0 || b >= shape[1]) return -1;
			if (c < 0 || c >= shape[2]) return -1;
			return  (a * shape[1] + b) * shape[2] + c;
		}

		inline sizetype Index(const int a, const int b, const int c, const int d)
		{
			const std::vector<int>& shape = Shape();
			if (a < 0 || a >= shape[0]) return -1;
			if (b < 0 || b >= shape[1]) return -1;
			if (c < 0 || c >= shape[2]) return -1;
			if (d < 0 || d >= shape[3]) return -1;
			return  ((a * shape[1] + b) * shape[2] + c) * shape[3] + d;
		}

		inline sizetype Index(const std::vector<int>& indices)
		{
			if (indices.empty()) return -1;
			const std::vector<int>& shape = Shape();
			size_t dim = shape.size();
			if (indices.size() != dim) return -1;
			if (indices[0] < 0 || indices[0] >= shape[0]) return -1;
			sizetype index = indices[0];
			for (size_t i = 1; i < dim; i++)
			{
				if (indices[i] < 0 || indices[i] >= shape[i]) return -1;
				index = index*shape[i] + indices[i];
			}
			return index;
		}

		T* GetCpu()
		{
			return (T*)impl->GetCpu();
		}

		T* GetGpu()
		{
			return (T*)impl->GetGpu();
		}

		bool Save(const std::wstring& path) override
		{
			return impl->Save(path);
		}

		bool Load(const std::wstring& path) override
		{
			return impl->Load(path);
		}
	};

	class CORE_API CPUReadonlyBuffer : public IBuffer
	{
	private:
		const void* address;
		size_t cur;
		size_t len;
		void ReadCore(void* addr, size_t size);
	public:
		CPUReadonlyBuffer() = delete;
		CPUReadonlyBuffer(const CPUReadonlyBuffer&) = delete;

		CPUReadonlyBuffer(const void* mem, size_t size);

		template<typename T>
		void Read(T* mem, size_t count)
		{
			ReadCore(mem, sizeof(T)*count);
		}

		template<typename T>
		void ReadBuffer(std::shared_ptr<GenericBuffer<T>> buf, const int a)
		{
			if (!buf) return;
			size_t size = a * sizeof(datatype);
			if (cur + size >= len) throw std::exception("out of buffer boundary");
			buf->Reshape(a);
			memcpy(buf->GetCpu(), (char*)address + cur, size);
		}

		template<typename T>
		void ReadBuffer(std::shared_ptr<GenericBuffer<T>> buf, const int a, const int b)
		{
			if (!buf) return;
			size_t size = a * b * sizeof(datatype);
			if (cur + size >= len) throw std::exception("out of buffer boundary");
			buf->Reshape(a, b);
			memcpy(buf->GetCpu(), (char*)address + cur, size);
		}

		template<typename T>
		void ReadBuffer(std::shared_ptr<GenericBuffer<T>> buf, const int a, const int b, const int c)
		{
			if (!buf) return;
			size_t size = a * b * c * sizeof(datatype);
			if (cur + size >= len) throw std::exception("out of buffer boundary");
			buf->Reshape(a, b, c);
			memcpy(buf->GetCpu(), (char*)address + cur, size);
		}

		template<typename T>
		void ReadBuffer(std::shared_ptr<GenericBuffer<T>> buf, const int a, const int b, const int c, const int d)
		{
			if (!buf) return;
			size_t size = a * b * c * d * sizeof(datatype);
			if (cur + size >= len) throw std::exception("out of buffer boundary");
			buf->Reshape(a, b, c, d);
			memcpy(buf->GetCpu(), (char*)address + cur, size);
		}

		void ReadBuffer(BufferPtr buf, const int a);
		void ReadBuffer(BufferPtr buf, const int a, const int b);
		void ReadBuffer(BufferPtr buf, const int a, const int b, const int c);
		void ReadBuffer(BufferPtr buf, const int a, const int b, const int c, const int d);

		std::wstring ReadString();

		void Seek(long long pos, SeekOrigin origin);

		bool IsEOF() const;
	};
	typedef std::shared_ptr<CPUReadonlyBuffer> CPUReadonlyBufferPtr;

	class BinaryBuffer_impl;
	class BinaryBuffer;
	typedef std::shared_ptr<BinaryBuffer> BinaryBufferPtr;

	class CORE_API BinaryBuffer : public IBuffer
	{
	private:
		std::shared_ptr<BinaryBuffer_impl> impl;
	public:
		BinaryBuffer();
		~BinaryBuffer();
		explicit BinaryBuffer(const sizetype a);

		const sizetype Shape() const;

		bool Reshape(sizetype shape);
		bool ReshapeLike(std::shared_ptr<BinaryBuffer> buf);

		inline sizetype Index(const sizetype a)
		{
			const sizetype shape = Shape();
			if (a < 0 || a >= shape) return -1;
			return  a;
		}

		void* GetCpu();
		void* GetGpu();

		CPUReadonlyBufferPtr GetCpuReadonlyBuffer();

		bool Save(const std::wstring& path) override;
		bool Load(const std::wstring& path) override;
	};

}