#pragma once
#include "TimeData.h"
#include "TimeSeriesLayer.h"

namespace rsdn
{
	namespace learning
	{
		namespace timeseries
		{
			class LEARNING_API TimeSeriesChannelSection : public IDataSection
			{
			public:
				BufferPtr Features;
				BufferPtr Times;
				std::shared_ptr<GenericBuffer<int>> Labels;
				TimeSeriesChannelSection();
			protected:
				boost::any GetDataCore(const std::wstring& key) override;
			};

			typedef std::shared_ptr<TimeSeriesChannelSection> TimeSeriesChannelSectionPtr;
			typedef std::vector<TimeSeriesChannelSectionPtr> TimeSeriesChannelSectionCollection;
			typedef std::shared_ptr<TimeSeriesChannelSectionCollection> TimeSeriesChannelSectionCollectionPtr;

			class LEARNING_API MultivariateTimeSeriesSection : public IDataSection
			{
			public:
				TimeSeriesChannelSectionCollectionPtr Channels;
				MultivariateTimeSeriesSection();
			public:
				__int64 GetChildCount() override;
				IDataSectionPtr GetChild(const __int64 index) override;
			};
			typedef std::shared_ptr<MultivariateTimeSeriesSection> MultivariateTimeSeriesSectionPtr;

			
			class LEARNING_API MultivariateTimeSeriesFeaturePacket : public TimeSeriesDataPacket
			{
			public:
				MultivariateTimeSeriesSectionPtr TemplateSection;

				MultivariateTimeSeriesFeaturePacket();

				REFLECT_CLASS(MultivariateTimeSeriesFeaturePacket)
			};
			typedef std::shared_ptr<MultivariateTimeSeriesFeaturePacket> MultivariateTimeSeriesFeaturePacketPtr;
		}
	}
}