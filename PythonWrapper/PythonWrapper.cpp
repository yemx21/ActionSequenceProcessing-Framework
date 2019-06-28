#include "Config.h"
#include "Functions.h"
using namespace boost::python;
using namespace rsdn::python;

namespace rsdn
{
	namespace python
	{

	}
}


BOOST_PYTHON_MODULE(rsdn)
{
	enum_<ComputationMode>("computationmode")
		.value("cpu", ComputationMode::CPU)
		.value("gpu", ComputationMode::GPU);

	class_<warpper_runtime, boost::noncopyable>("runtime")
		.def("initialize", &warpper_runtime::initialize)
		.def("shutdown", &warpper_runtime::shutdown)
		.add_property("cpujobcount", &warpper_runtime::getcpujobcount, &warpper_runtime::setcpujobcount)
		.add_property("mode", &warpper_runtime::getmode, &warpper_runtime::setmode);

	class_<warpper_operator, boost::noncopyable>("operator")
		.def(init<std::wstring>());

	class_<warpper_layer, boost::noncopyable>("layer")
		.def(init<std::wstring>())
		.def("operators", range(&warpper_layer::begin, &warpper_layer::end));

	class_<warpper_graph, boost::noncopyable>("graph")
		.def(init<>())
		.def(init<std::wstring>())
		.def("run", &warpper_graph::run)
		.def("layers", range(&warpper_graph::begin, &warpper_graph::end));


}



