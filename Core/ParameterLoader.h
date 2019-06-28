#pragma once
#include "Layer.h"

namespace rsdn
{
	namespace data
	{
		enum class ParameterBaseType:int
		{
			Unknown = -1,
			None,
			Boolean,
			Int8,
			UInt8,
			Int16,
			UInt16,
			Int32,
			UInt32,
			Int64,
			UInt64,
			Float,
			Double,
			String,
		};

		enum class ParameterBufferType:int
		{
			Unknown=-1,
			None,
			Buffer,
			GenericBuffer,
			BinaryBuffer
		};

		class CORE_API ParameterStringReader
		{
		public:
			static ParameterPacketPtr CreateFromString(const std::wstring& strings, std::wstring& err);
			static bool AttachTo(ParameterPacketPtr params, const std::wstring& strings, std::wstring& err);

			static ParameterPacketPtr CreateFromString(const std::wstring& paramname, const std::wstring& valexpression, std::wstring& err);
			static bool AttachTo(ParameterPacketPtr params, const std::wstring& paramname, const std::wstring& valexpression, std::wstring& err);

		};
	}
}
