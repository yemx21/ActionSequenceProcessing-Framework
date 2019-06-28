#pragma once
#include "Config.h"
#include "..\Core\Core.h"
#include "..\Graphics\Graphics.h"

namespace rsdn
{
	namespace data
	{
		class PythonParameterPacket : public ParameterPacket
		{
		private:
			std::map<rsdn::InertiaString, boost::any> params;
			std::map<int, rsdn::InertiaString> indices;
			std::map<rsdn::InertiaString, IBufferPtr> buffers;
		protected:
			int GetCountCore() const final override;

			ParameterItemType GetTypeCore(const std::wstring& key) const final override;
			ParameterItemType GetTypeCore(int index) const final override;

			const boost::any& GetCore(const std::wstring& key) const final override;
			const boost::any& GetAtIndexCore(int index) const final override;;

			IBufferPtr GetCoreEx(const std::wstring& key) const final override;;
			IBufferPtr GetAtIndexCoreEx(int index) const final override;;

		public:
			bool Update(boost::python::dict& params);

			REFLECT_CLASS(PythonParameterPacket)
		};
		typedef std::shared_ptr<PythonParameterPacket> PythonParameterPacketPtr;
	}
}
