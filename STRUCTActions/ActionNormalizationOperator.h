#pragma once
#include "ActionRepresentationLayer.h"
#include "ActionDataPacket.h"
#include "ActionRepresentationDataPacket.h"
#include <concurrent_vector.h>
#include <concurrent_unordered_map.h>

namespace rsdn
{
	namespace learning
	{
		namespace timeseries
		{
			namespace operators
			{
				class ActionNormalizationOperator : public ActionRepresentationLayerOperator
				{
				private:
					bool isexclusive;
					int batchcount;
					std::vector<std::wstring> batches;
					std::map<int, std::wstring> featsamples;
				private:
					ResultPtr RunCPU(ActionDataPacketPtr indata, int desiredframes, int sr, ActionRepresentationDataPacketPtr outdata, std::atomic<bool>& cancel);
				public:
					ActionNormalizationOperator();
					ResultPtr Run(ActionDataPacketPtr indata, data::ParameterPacketPtr inparam, _type intype, data::ParameterPacketPtr param, ActionRepresentationDataPacketPtr outdata, std::atomic<bool>& cancel) override final;
				protected:
					data::ParameterPacketPtr params;
					ResultPtr ConfigCore(data::ParameterPacketPtr param) override;

					bool IsExclusive() const override;
					unsigned int GetBatchCount() override;
					ResultPtr PrepareBatchAsync(ActionDataPacketPtr indata, data::ParameterPacketPtr inparam, _type intype, data::ParameterPacketPtr param, ActionRepresentationDataPacketPtr outdata, std::atomic<bool>& cancel) override;
					ResultPtr RunBatchAsync(ActionDataPacketPtr indata, data::ParameterPacketPtr inparam, _type intype, data::ParameterPacketPtr param, ActionRepresentationDataPacketPtr outdata, std::atomic<bool>& cancel, unsigned int batch) override;

					REFLECT_CLASS(ActionNormalizationOperator)
				};
			}
		}
	}
}

