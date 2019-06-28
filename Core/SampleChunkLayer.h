#pragma once
#include "DataLayer.h"
#include "Sample.h"

using namespace rsdn;

namespace rsdn
{
	namespace data
	{
		class CORE_API SampleChunkDataSection : public IDataSection
		{
		public:
			std::unique_ptr<SampleChunk> Samples;
		protected:
			void* GetDataCore1(const std::wstring& key) override;
		public:
			SampleChunkDataSection();
		};
		typedef std::shared_ptr<SampleChunkDataSection> SampleChunkDataSectionPtr;

		class CORE_API SampleChunkDataPacket : public DataPacket
		{
		public:
			SampleChunkDataSectionPtr Samples;

			SampleChunkDataPacket();

			REFLECT_CLASS(SampleChunkDataPacket)
		};
		typedef std::shared_ptr<SampleChunkDataPacket> SampleChunkDataPacketPtr;


		class CORE_API SampleChunkLayer : public layer::DataLayer
		{
		private:
			std::wstring inpath;
			SampleChunkDataPacketPtr outpacket;
		public:
			SampleChunkLayer();
			~SampleChunkLayer();

			virtual bool Open(const std::wstring& path);

		protected:
			virtual std::unique_ptr<std::future<ResultPtr>> RunAsync(std::atomic<bool>& cancel) override;

			ResultPtr ReadyCore() override final;
			ResultPtr IsSupportConnectTo(_type next) override final;
			ResultPtr OutCore(data::DataPacketPtr& data, data::ParameterPacketPtr& parameter, _type next) override final;


			REFLECT_CLASS(SampleChunkLayer)
		};
	}
}