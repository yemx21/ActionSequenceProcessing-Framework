#pragma once
#include "FileStream.h"
#include "BSDB.h"
#include "Layer.h"
#include <memory>
#include <string>


using namespace rsdn;

namespace rsdn
{
	namespace layer
	{
		class CORE_API DataLayer: public Layer
		{
		protected:
			FileStreamPtr file;
			data::BSDBReaderPtr bsdb;

		public:
			DataLayer();
			~DataLayer();

			virtual bool Open(const std::wstring& path);

		protected:
			virtual std::unique_ptr<std::future<ResultPtr>> RunAsync(std::atomic<bool>& cancel) override;

			REFLECT_CLASS(DataLayer)
		};

		typedef std::shared_ptr<DataLayer> DataLayerPtr;

	}
}