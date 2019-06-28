#pragma once
#include "ParameterLoader.h"
#include "InertiaString.h"

namespace rsdn
{
	namespace data
	{
		class StringDerivedParameterPacket : public ParameterPacket
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

			ParameterBaseType FindBaseType(const std::wstring& name);
			ParameterBufferType FindBufferType(const std::wstring& name);
			bool TryParse(ParameterBaseType base, const std::wstring& eval, boost::any& obj);
			bool TryParseBuffer(ParameterBaseType base, ParameterBufferType ty, const std::wstring& size, const std::wstring& eval, IBufferPtr& obj);
		public:
			bool TryAddByString(const std::wstring& expression, std::wstring& err);

			bool TryAddByString(const std::wstring& paramname, const std::wstring& valueexpression, std::wstring& err);

			REFLECT_CLASS(StringDerivedParameterPacket)
		};
		typedef std::shared_ptr<StringDerivedParameterPacket> StringDerivedParameterPacketPtr;
	}
}
