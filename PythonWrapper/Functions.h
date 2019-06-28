#pragma once
#include "..\Core\Core.h"
#include "..\Graphics\Graphics.h"
#include "Config.h"

namespace rsdn
{
	namespace python
	{
		class warpper_runtime
		{
		public:
			void initialize();
			void shutdown();

			int getcpujobcount() const;
			void setcpujobcount(int count);

			ComputationMode getmode() const;
			void setmode(ComputationMode mode);
		};

		class warpper_operator: boost::noncopyable
		{
		private:
			OperatorPtr native_ptr;
			boost::python::dict native_params;
		public:
			warpper_operator(const std::wstring& name);

			boost::python::dict& getparams();

			void updateparams();
		};

		class warpper_layer : boost::noncopyable
		{
		private:
			Layer* native_ptr;
			boost::python::dict native_params;
		public:
			warpper_layer(const std::wstring& name);

			std::vector<warpper_operator>::iterator begin();
			std::vector<warpper_operator>::iterator end();

			boost::python::dict& getparams();

			void updateparams();
		};

		class warpper_graph : boost::noncopyable
		{
		private:
			GraphPtr native_ptr;
			std::vector<warpper_layer> layers;
		public:
			warpper_graph();

			warpper_graph(const std::wstring& path);
			void run(bool standardalonefirst);

			std::vector<warpper_layer>::iterator begin();
			std::vector<warpper_layer>::iterator end();

			void updateparams();
		};
	}
}

