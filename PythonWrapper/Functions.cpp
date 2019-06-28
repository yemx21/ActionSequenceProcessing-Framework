#include "Functions.h"

using namespace rsdn::python;
using namespace rsdn::graphics;

void warpper_runtime::initialize()
{
	Random::Initilize();
	Graphics::Initilize();
}

void warpper_runtime::shutdown()
{
	Random::Shutdown();
	Graphics::Shutdown();
}

int warpper_runtime::getcpujobcount() const
{
	return Runtime::Get().GetCpuJobCount();
}

void warpper_runtime::setcpujobcount(int count)
{
	Runtime::Get().SetCpuJobCount(count);
}

ComputationMode warpper_runtime::getmode() const
{
	return Runtime::Get().GetMode();
}

void warpper_runtime::setmode(ComputationMode mode)
{
	Runtime::Get().SetMode(mode);
}

warpper_graph::warpper_graph(const std::wstring& path)
{ 
	rsdn::ResultPtr ret;
	native_ptr = Graph::LoadFromFile(path, ret, std::locale());
	if (!ret->State || !native_ptr)
	{
		Logger::Get().Report(LogLevel::Info) << "msg: " << ret->Message << "\r\nerr:" << ret->Error << Logger::endl;
		throw std::exception("can not load graph file");
	}	
	Logger::Get().Report(LogLevel::Info) << "graph file loaded" << Logger::endl;
}

void warpper_graph::run(bool standardalonefirst)
{
	auto ret = native_ptr->RunAsync(standardalonefirst);
	if (ret) ret->Wait();
	auto fret = ret->Get();
	if (!fret->State)
	{
		Logger::Get().Report(LogLevel::Info) << "msg: " << fret->Message << "\r\nerr:" << fret->Error << Logger::endl;
	}
	Logger::Get().Report(LogLevel::Info) << "done" << Logger::endl;
}


warpper_operator::warpper_operator(const std::wstring& name)
{
	native_ptr = TypeFactory::CreateInstance<Operator>(name);
}

boost::python::dict& warpper_operator::getparams()
{
	return native_params;
}

void warpper_operator::updateparams()
{
	boost::python::dict::get
}