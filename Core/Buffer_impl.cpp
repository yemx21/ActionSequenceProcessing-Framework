#include "Buffer_impl.h"
using namespace rsdn;

inline void rsdn_mallocHost(void** ptr, size_t size, bool* use_cuda) 
{
	if (Runtime::Mode() == ComputationMode::GPU) 
	{
		cudaMallocHost(ptr, size);
		*use_cuda = true;
		return;
	}

	*ptr = malloc(size);
	*use_cuda = false;
}

inline void rsdn_freeHost(void* ptr, bool use_cuda)
{
	if (use_cuda) 
	{
		cudaFreeHost(ptr);
		return;
	}
	free(ptr);
}

#pragma region SwapBuffer
SwapBuffer::SwapBuffer():cpudata(nullptr), gpudata(nullptr), gpushape(nullptr), cpu(false), gpu(false), cpu_usecuda(false), memsize(0), state(bufferstate::uninitialized)
{
	cudaGetDevice(&gpudevice);
}

SwapBuffer::SwapBuffer(sizetype size) : cpudata(nullptr), gpudata(nullptr), gpushape(nullptr), cpu(false), gpu(false), cpu_usecuda(false), memsize(size), state(bufferstate::uninitialized)
{
	cudaGetDevice(&gpudevice);
}

SwapBuffer::~SwapBuffer()
{
	if (cpudata && cpu) 
	{
		rsdn_freeHost(cpudata, cpu_usecuda);
	}

	if (gpudata && gpu) 
	{
		cudaFree(gpudata);
	}
}

void SwapBuffer::toCPU()
{
	switch (state) 
	{
	case bufferstate::uninitialized:
		rsdn_mallocHost(&cpudata, memsize, &cpu_usecuda);
		memset(cpudata, 0, memsize);
		state = bufferstate::cpu;
		cpu = true;
		break;
	case bufferstate::gpu:
		if (!cpudata) 
		{
			rsdn_mallocHost(&cpudata, memsize, &cpu_usecuda);
			cpu = true;
		}
		cudaMemcpy(cpudata, gpudata, memsize, cudaMemcpyDefault);
		state = bufferstate::synced;
		break;
	case bufferstate::cpu:
	case bufferstate::synced:
		break;
	}
}

void SwapBuffer::toGPU()
{
	switch (state) 
	{
	case bufferstate::uninitialized:
		cudaMalloc(&gpudata, memsize);
		cudaMemset(gpudata, 0, memsize);
		state = bufferstate::gpu;
		gpu = true;
		break;
	case bufferstate::cpu:
		if (!gpudata) 
		{
			cudaMalloc(&gpudata, memsize);
			gpu = true;
		}
		cudaMemcpy(gpudata, cpudata, memsize, cudaMemcpyDefault);
		state = bufferstate::synced;
		break;
	case bufferstate::gpu:
	case bufferstate::synced:
		break;
	}
}


const void* SwapBuffer::getcpu()
{
	toCPU();
	return cpudata;
}

const void* SwapBuffer::getgpu()
{
	toGPU();
	return gpudata;
}

void SwapBuffer::setcpu(void* data)
{
	if (cpu) 
	{
		rsdn_freeHost(cpudata, cpu_usecuda);
	}
	cpudata = data;
	state = bufferstate::cpu;
	cpu = false;
}

void SwapBuffer::setgpu(void* data) 
{
	if (gpu) 
	{
		cudaFree(gpudata);
	}
	gpudata = data;
	state = bufferstate::gpu;
	gpu = false;
}

void* SwapBuffer::mutablecpu()
{
	toCPU();
	state = bufferstate::cpu;
	return cpudata;
}

void* SwapBuffer::mutablegpu()
{
	toGPU();
	state = bufferstate::gpu;
	return gpudata;
}

void SwapBuffer::async_gpupush(const cudaStream_t& stream)
{
	if (state != bufferstate::cpu) return;
	if (!gpudata) 
	{
		cudaMalloc(&gpudata, memsize);
		gpu = true;
	}
	const cudaMemcpyKind put = cudaMemcpyHostToDevice;
	cudaMemcpyAsync(gpudata, cpudata, memsize, put, stream);
	state = bufferstate::synced;
}

#pragma endregion

#pragma region Buffer_impl
Buffer_impl::Buffer_impl() : data(), diff(), count(0), capacity(0)
{

}

Buffer_impl::Buffer_impl(const int num, const int channels, const int height, const int width) : capacity(0)
{
	Reshape(num, channels, height, width);
}

Buffer_impl::Buffer_impl(const std::vector<int>& shapevec) : capacity(0)
{
	Reshape(shapevec);
}

void Buffer_impl::ReshapeLike(const Buffer_impl& other) 
{
	Reshape(other.shape);
}

void Buffer_impl::Reshape(const int num, const int channels, const int height, const int width)
{
	std::vector<int> shapevec(4);
	shapevec[0] = num;
	shapevec[1] = channels;
	shapevec[2] = height;
	shapevec[3] = width;
	Reshape(shapevec);
}

void Buffer_impl::Reshape(const std::vector<int>& shapevec)
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
		data.reset(new SwapBuffer(capacity * sizeof(datatype)));
		diff.reset(new SwapBuffer(capacity * sizeof(datatype)));
	}
}

bool Buffer_impl::IsShapeSameAs(const Buffer_impl& other)
{
	if (shape.size() != other.shape.size()) return false;
	for (size_t i = 0; i < shape.size(); i++)
	{
		if (shape[i] != other.shape[i]) return false;
	}
	return true;
}

datatype* Buffer_impl::GetCpu()
{
	return data ? (datatype*)data->mutablecpu() : nullptr;
}

datatype* Buffer_impl::GetGpu()
{
	return data ? (datatype*)data->mutablegpu() : nullptr;
}

#pragma endregion

#pragma region BinaryBuffer_impl
BinaryBuffer_impl::BinaryBuffer_impl() : data(), count(0), capacity(0)
{

}

BinaryBuffer_impl::BinaryBuffer_impl(const sizetype shape) : capacity(0)
{
	Reshape(shape);
}

void BinaryBuffer_impl::ReshapeLike(const BinaryBuffer_impl& other)
{
	Reshape(other.shape);
}

void BinaryBuffer_impl::Reshape(const sizetype shapevec)
{
	count = 0;
	if (!shapedata || shapedata->memsize < sizeof(int))
	{
		shapedata.reset(new SwapBuffer(sizeof(int)));
	}
	int* shape_data = static_cast<int*>(shapedata->mutablecpu());
	if (shapevec < 0) throw std::exception("wrong shape size");
	if (count != 0)
	{
		if (shapevec > INT_MAX / count) throw std::exception("shape size exceeds INT_MAX");
	}
	count = shapevec;
	shape = shapevec;
	shape_data[0] = shapevec;
	if (count > capacity)
	{
		capacity = count;
		data.reset(new SwapBuffer(capacity));
	}
}

void* BinaryBuffer_impl::GetCpu()
{
	return data ? data->mutablecpu() : nullptr;
}

void* BinaryBuffer_impl::GetGpu()
{
	return data ? data->mutablegpu() : nullptr;
}

#pragma endregion


#pragma region FixedBuffer
FixedBuffer::FixedBuffer() :cpudata(nullptr), gpudata(nullptr), gpushape(nullptr), cpu(false), gpu(false), cpu_usecuda(false), memsize(0), state(bufferstate::uninitialized)
{
	cudaGetDevice(&gpudevice);
}

FixedBuffer::FixedBuffer(sizetype size) : cpudata(nullptr), gpudata(nullptr), gpushape(nullptr), cpu(false), gpu(false), cpu_usecuda(false), memsize(size), state(bufferstate::uninitialized)
{
	cudaGetDevice(&gpudevice);
}

FixedBuffer::~FixedBuffer()
{
	if (cpudata && cpu)
	{
		rsdn_freeHost(cpudata, cpu_usecuda);
	}

	if (gpudata && gpu)
	{
		cudaFree(gpudata);
	}
}

void FixedBuffer::toCPU()
{
	switch (state)
	{
	case bufferstate::uninitialized:
		rsdn_mallocHost(&cpudata, memsize, &cpu_usecuda);
		memset(cpudata, 0, memsize);
		state = bufferstate::cpu;
		cpu = true;
		break;
	case bufferstate::gpu:
		if (!cpudata)
		{
			rsdn_mallocHost(&cpudata, memsize, &cpu_usecuda);
			cpu = true;
		}
		cudaMemcpy(cpudata, gpudata, memsize, cudaMemcpyDefault);
		state = bufferstate::synced;
		break;
	case bufferstate::cpu:
	case bufferstate::synced:
		break;
	}
}

void FixedBuffer::toGPU()
{
	switch (state)
	{
	case bufferstate::uninitialized:
		cudaMalloc(&gpudata, memsize);
		cudaMemset(gpudata, 0, memsize);
		state = bufferstate::gpu;
		gpu = true;
		break;
	case bufferstate::cpu:
		if (!gpudata)
		{
			cudaMalloc(&gpudata, memsize);
			gpu = true;
		}
		cudaMemcpy(gpudata, cpudata, memsize, cudaMemcpyDefault);
		state = bufferstate::synced;
		break;
	case bufferstate::gpu:
	case bufferstate::synced:
		break;
	}
}


const void* FixedBuffer::getcpu()
{
	toCPU();
	return cpudata;
}

const void* FixedBuffer::getgpu()
{
	toGPU();
	return gpudata;
}

void FixedBuffer::setcpu(void* data)
{
	if (cpu)
	{
		rsdn_freeHost(cpudata, cpu_usecuda);
	}
	cpudata = data;
	state = bufferstate::cpu;
	cpu = false;
}

void FixedBuffer::setgpu(void* data)
{
	if (gpu)
	{
		cudaFree(gpudata);
	}
	gpudata = data;
	state = bufferstate::gpu;
	gpu = false;
}

void* FixedBuffer::mutablecpu()
{
	toCPU();
	state = bufferstate::cpu;
	return cpudata;
}

void* FixedBuffer::mutablegpu()
{
	toGPU();
	state = bufferstate::gpu;
	return gpudata;
}

void FixedBuffer::async_gpupush(const cudaStream_t& stream)
{
	if (state != bufferstate::cpu) return;
	if (!gpudata)
	{
		cudaMalloc(&gpudata, memsize);
		gpu = true;
	}
	const cudaMemcpyKind put = cudaMemcpyHostToDevice;
	cudaMemcpyAsync(gpudata, cpudata, memsize, put, stream);
	state = bufferstate::synced;
}

#pragma endregion

