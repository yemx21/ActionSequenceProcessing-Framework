#include "TimeSeriesLayer.h"
#include "TimeData.h"
#include "MultivariateTimeSeriesFeaturePacket.h"
#include "MultivariateTimeSeriesFeatureExtractionLayer.h"
#include "PeriodicClusteringOperator.h"
#include "PeriodicPairwiseFeatureExtractionOperator.h"
#include "RandomForestLayerDataPacket.h"
#include "RandomForestLayer.h"
#include "PairwiseFieldOperator.h"

#pragma region rsdn::learning::timeseries::TimeSeriesLayer
__uregister_class1(rsdn::learning::timeseries::TimeSeriesLayer, TimeSeriesLayer)
{
	type.AddBaseObject<rsdn::layer::Layer>();

	TypeFactory::Regisiter(&type);
}
#pragma endregion

#pragma region rsdn::learning::timeseries::TimeSeriesDataPacket
__uregister_class1(rsdn::learning::timeseries::TimeSeriesDataPacket, TimeSeriesDataPacket)
{
	type.AddBaseObject<rsdn::data::DataPacket>();

	TypeFactory::Regisiter(&type);
}
#pragma endregion


#pragma region rsdn::learning::timeseries::MultivariateTimeSeriesFeaturePacket
__uregister_class1(rsdn::learning::timeseries::MultivariateTimeSeriesFeaturePacket, MultivariateTimeSeriesFeaturePacket)
{
	type.AddBaseObject<rsdn::learning::timeseries::TimeSeriesDataPacket>();

	TypeFactory::Regisiter(&type);
}
#pragma endregion

#pragma region rsdn::learning::timeseries::MultivariateTimeSeriesFeatureExtractionLayer
__uregister_class1(rsdn::learning::timeseries::MultivariateTimeSeriesFeatureExtractionLayer, MultivariateTimeSeriesFeatureExtractionLayer)
{
	type.AddBaseObject<rsdn::learning::timeseries::TimeSeriesLayer>();

	TypeFactory::Regisiter(&type);
}
#pragma endregion


#pragma region rsdn::learning::timeseries::MultivariateTimeSeriesFeatureExtractionLayerOperator
__uregister_class1(rsdn::learning::timeseries::MultivariateTimeSeriesFeatureExtractionLayerOperator, MultivariateTimeSeriesFeatureExtractionLayerOperator)
{
	type.AddBaseObject<rsdn::layer::Operator>();

	TypeFactory::Regisiter(&type);
}
#pragma endregion

#pragma region rsdn::learning::timeseries::PeriodicClusteringOperator
__uregister_class1(rsdn::learning::timeseries::operators::PeriodicClusteringOperator, PeriodicClusteringOperator)
{
	type.AddBaseObject<rsdn::learning::timeseries::MultivariateTimeSeriesFeatureExtractionLayerOperator>();

	TypeFactory::Regisiter(&type);
}
#pragma endregion

#pragma region rsdn::learning::timeseries::PeriodicPairwiseFeatureExtractionOperator
__uregister_class1(rsdn::learning::timeseries::operators::PeriodicPairwiseFeatureExtractionOperator, PeriodicPairwiseFeatureExtractionOperator)
{
	type.AddBaseObject<rsdn::learning::timeseries::MultivariateTimeSeriesFeatureExtractionLayerOperator>();

	TypeFactory::Regisiter(&type);
}
#pragma endregion

#pragma region rsdn::learning::timeseries::PairwiseFieldOperator
__uregister_class1(rsdn::learning::timeseries::operators::PairwiseFieldOperator, PairwiseFieldOperator)
{
	type.AddBaseObject<rsdn::learning::timeseries::MultivariateTimeSeriesFeatureExtractionLayerOperator>();

	TypeFactory::Regisiter(&type);
}
#pragma endregion


#pragma region rsdn::learning::machines::supervised::RandomForestLayer
__uregister_class1(rsdn::learning::machines::supervised::RandomForestLayer, RandomForestLayer)
{
	type.AddBaseObject<rsdn::layer::Layer>();

	TypeFactory::Regisiter(&type);
}
#pragma endregion

#pragma region rsdn::learning::machines::supervised::RandomForestLayerDataPacket
__uregister_class1(rsdn::learning::machines::supervised::RandomForestLayerDataPacket, RandomForestLayerDataPacket)
{
	type.AddBaseObject<rsdn::data::DataPacket>();

	TypeFactory::Regisiter(&type);
}
#pragma endregion


