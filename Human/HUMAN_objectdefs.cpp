#include "SkeletonDataPacket.h"
#include "SkeletonDataLayer.h"
#include "SkeletonRepresentationDataPacket.h"
#include "SkeletonRepresentationLayer.h"
#include "GaitCycleNormalizationOperator.h"
#include "GaitPhaseRepresentationOperator.h"

#pragma region rsdn::learning::timeseries::SkeletonDataLayer
__uregister_class1(rsdn::learning::timeseries::SkeletonDataLayer, SkeletonDataLayer)
{
	type.AddBaseObject<rsdn::learning::timeseries::TimeSeriesLayer>();

	TypeFactory::Regisiter(&type);
}
#pragma endregion

#pragma region rsdn::learning::timeseries::SkeletonDataPacket
__uregister_class1(rsdn::learning::timeseries::SkeletonDataPacket, SkeletonDataPacket)
{
	type.AddBaseObject<rsdn::learning::timeseries::TimeSeriesDataPacket>();

	TypeFactory::Regisiter(&type);
}
#pragma endregion

#pragma region rsdn::learning::timeseries::SkeletonRepresentationLayerOperator
__uregister_class1(rsdn::learning::timeseries::SkeletonRepresentationLayerOperator, SkeletonRepresentationLayerOperator)
{
	type.AddBaseObject<rsdn::layer::Operator>();

	TypeFactory::Regisiter(&type);
}
#pragma endregion

#pragma region rsdn::learning::timeseries::SkeletonRepresentationLayer
__uregister_class1(rsdn::learning::timeseries::SkeletonRepresentationLayer, SkeletonRepresentationLayer)
{
	type.AddBaseObject<rsdn::learning::timeseries::TimeSeriesLayer>();

	TypeFactory::Regisiter(&type);
}
#pragma endregion

#pragma region rsdn::learning::timeseries::SkeletonRepresentationDataPacket
__uregister_class1(rsdn::learning::timeseries::SkeletonRepresentationDataPacket, SkeletonRepresentationDataPacket)
{
	type.AddBaseObject<rsdn::learning::timeseries::TimeSeriesDataPacket>();

	TypeFactory::Regisiter(&type);
}
#pragma endregion

#pragma region rsdn::learning::timeseries::operators::GaitPhaseRepresentationOperator
__uregister_class1(rsdn::learning::timeseries::operators::GaitPhaseRepresentationOperator, GaitPhaseRepresentationOperator)
{
	type.AddBaseObject<rsdn::learning::timeseries::SkeletonRepresentationLayerOperator>();

	TypeFactory::Regisiter(&type);
}
#pragma endregion

#pragma region rsdn::learning::timeseries::operators::GaitCycleNormalizationOperator
__uregister_class1(rsdn::learning::timeseries::operators::GaitCycleNormalizationOperator, GaitCycleNormalizationOperator)
{
	type.AddBaseObject<rsdn::learning::timeseries::SkeletonRepresentationLayerOperator>();

	TypeFactory::Regisiter(&type);
}
#pragma endregion

