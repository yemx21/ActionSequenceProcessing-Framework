#pragma once
#include "Buffer.h"
#include "cuda_config.h"
#include <vector>
namespace rsdn
{
	class SwapBuffer
	{
	private:
		enum class bufferstate
		{
			uninitialized,
			cpu,
			gpu,
			synced,
		};

		void toCPU();
		void toGPU();

	public:
		bool cpu;
		bool cpu_usecuda;
		void* cpudata;

		bool gpu;
		void* gpudata;
		int gpudevice;

		std::vector<int> cpushape;
		int* gpushape;

		sizetype memsize;
		bufferstate state;

		SwapBuffer();
		SwapBuffer(sizetype size);
		~SwapBuffer();

		const void* getcpu();
		const void* getgpu();

		void* mutablecpu();
		void* mutablegpu();

		void setcpu(void* data);
		void setgpu(void* data);

		void async_gpupush(const cudaStream_t& stream);

		template<typename T>
		static inline sizetype capacity(const std::vector<int>& shape)
		{
			if (shape.empty()) return 0;
			sizetype memsize = 1;
			for (int s : shape)
			{
				memsize *= s;
			}
			return memsize * sizeof(T);
		}

		DISABLE_COPY_AND_ASSIGN(SwapBuffer);
	};

	class Buffer_impl
	{
	public:
		std::shared_ptr<SwapBuffer> data;
		std::shared_ptr<SwapBuffer> diff;
		std::shared_ptr<SwapBuffer> shapedata;
		std::vector<int> shape;
		int count;
		int capacity;

		Buffer_impl();
		explicit Buffer_impl(const int num, const int channels, const int height, const int width);
		explicit Buffer_impl(const std::vector<int>& shape);

		void Reshape(const int num, const int channels, const int height, const int width);
		void Reshape(const std::vector<int>& shape);
		void ReshapeLike(const Buffer_impl& other);
		bool IsShapeSameAs(const Buffer_impl& other);

		datatype* GetCpu();
		datatype* GetGpu();

		DISABLE_COPY_AND_ASSIGN(Buffer_impl);
	};

	class BinaryBuffer_impl
	{
	public:
		std::shared_ptr<SwapBuffer> data;
		std::shared_ptr<SwapBuffer> shapedata;
		sizetype shape;
		sizetype count;
		sizetype capacity;

		BinaryBuffer_impl();
		explicit BinaryBuffer_impl(const sizetype num);

		void Reshape(const sizetype shape);
		void ReshapeLike(const BinaryBuffer_impl& other);

		void* GetCpu();
		void* GetGpu();

		DISABLE_COPY_AND_ASSIGN(BinaryBuffer_impl);
	};

	class FixedBuffer
	{
	private:
		enum class bufferstate
		{
			uninitialized,
			cpu,
			gpu,
			synced,
		};

		void toCPU();
		void toGPU();

	public:
		bool cpu;
		bool cpu_usecuda;
		void* cpudata;

		bool gpu;
		void* gpudata;
		int gpudevice;

		std::vector<int> cpushape;
		int* gpushape;

		sizetype memsize;
		bufferstate state;

		FixedBuffer();
		FixedBuffer(sizetype size);
		~FixedBuffer();

		const void* getcpu();
		const void* getgpu();

		void* mutablecpu();
		void* mutablegpu();

		void setcpu(void* data);
		void setgpu(void* data);

		void async_gpupush(const cudaStream_t& stream);

		template<typename T>
		static inline sizetype capacity(const std::vector<int>& shape)
		{
			if (shape.empty()) return 0;
			sizetype memsize = 1;
			for (int s : shape)
			{
				memsize *= s;
			}
			return memsize * sizeof(T);
		}

		DISABLE_COPY_AND_ASSIGN(FixedBuffer);
	};
}
