#include "ActionDataPacket.h"
#include "STRUCTDataLayer.h"
#include "ActionRepresentationDataPacket.h"
#include "ActionRepresentationLayer.h"
#include "ActionNormalizationOperator.h"
#include "ActionNormalsDataConversionLayer.h"
#include "ActionEventPairLayer.h"
#include "ActionPairwiseLayer.h"
#include "ActionNormalsSamplerLayer.h"


#pragma region rsdn::learning::timeseries::STRUCTDataLayer
__uregister_class1(rsdn::learning::timeseries::STRUCTDataLayer, STRUCTDataLayer)
{
	type.AddBaseObject<rsdn::learning::timeseries::TimeSeriesLayer>();

	TypeFactory::Regisiter(&type);
}
#pragma endregion

#pragma region rsdn::learning::timeseries::ActionDataPacket
__uregister_class1(rsdn::learning::timeseries::ActionDataPacket, ActionDataPacket)
{
	type.AddBaseObject<rsdn::learning::timeseries::TimeSeriesDataPacket>();

	TypeFactory::Regisiter(&type);
}
#pragma endregion

#pragma region rsdn::learning::timeseries::ActionRepresentationDataPacket
__uregister_class1(rsdn::learning::timeseries::ActionRepresentationDataPacket, ActionRepresentationDataPacket)
{
	type.AddBaseObject<rsdn::learning::timeseries::TimeSeriesDataPacket>();

	TypeFactory::Regisiter(&type);
}
#pragma endregion

#pragma region rsdn::learning::timeseries::ActionRepresentationLayer
__uregister_class1(rsdn::learning::timeseries::ActionRepresentationLayer, ActionRepresentationLayer)
{
	type.AddBaseObject<rsdn::learning::timeseries::TimeSeriesLayer>();

	TypeFactory::Regisiter(&type);
}
#pragma endregion

#pragma region rsdn::learning::timeseries::ActionRepresentationLayerOperator
__uregister_class1(rsdn::learning::timeseries::ActionRepresentationLayerOperator, ActionRepresentationLayerOperator)
{
	type.AddBaseObject<rsdn::layer::Operator>();

	TypeFactory::Regisiter(&type);
}
#pragma endregion

#pragma region rsdn::learning::timeseries::operators::ActionNormalizationOperator
__uregister_class1(rsdn::learning::timeseries::operators::ActionNormalizationOperator, ActionNormalizationOperator)
{
	type.AddBaseObject<rsdn::learning::timeseries::ActionRepresentationLayerOperator>();

	TypeFactory::Regisiter(&type);
}
#pragma endregion

#pragma region rsdn::learning::timeseries::ActionNormalsDataConversionLayer
__uregister_class1(rsdn::learning::timeseries::ActionNormalsDataConversionLayer, ActionNormalsDataConversionLayer)
{
	type.AddBaseObject<rsdn::layer::Layer>();

	TypeFactory::Regisiter(&type);
}
#pragma endregion


#pragma region rsdn::learning::timeseries::ActionEventPairLayer
__uregister_class1(rsdn::learning::timeseries::ActionEventPairLayer, ActionEventPairLayer)
{
	type.AddBaseObject<rsdn::layer::Layer>();

	TypeFactory::Regisiter(&type);
}
#pragma endregion

#pragma region rsdn::learning::timeseries::ActionPairwiseLayer
__uregister_class1(rsdn::learning::timeseries::ActionPairwiseLayer, ActionPairwiseLayer)
{
	type.AddBaseObject<rsdn::layer::Layer>();

	TypeFactory::Regisiter(&type);
}
#pragma endregion


#pragma region rsdn::learning::timeseries::ActionNormalsSamplerLayer
__uregister_class1(rsdn::learning::timeseries::ActionNormalsSamplerLayer, ActionNormalsSamplerLayer)
{
	type.AddBaseObject<rsdn::layer::Layer>();

	TypeFactory::Regisiter(&type);
}
#pragma endregion



