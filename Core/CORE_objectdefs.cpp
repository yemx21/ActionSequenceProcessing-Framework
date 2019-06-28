#include "Layer.h"
#include "DataLayer.h"
#include "StringDerivedParameterPacket.h"
#include "SampleChunkLayer.h"

#pragma region rsdn::data::DataPacket
__uregister_class1(rsdn::data::DataPacket, DataPacket)
{
	TypeFactory::Regisiter(&type);
}
#pragma endregion

#pragma region rsdn::data::ParameterPacket
__uregister_class1(rsdn::data::ParameterPacket, ParameterPacket)
{
	TypeFactory::Regisiter(&type);
}
#pragma endregion

#pragma region rsdn::layer::Operator
__uregister_class1(rsdn::layer::Operator, Operator)
{
	TypeFactory::Regisiter(&type);
}
#pragma endregion

#pragma region rsdn::layer::Layer
__uregister_class1(rsdn::layer::Layer, Layer)
{
	TypeFactory::Regisiter(&type);
}
#pragma endregion

#pragma region rsdn::layer::DataLayer
__uregister_class1(rsdn::layer::DataLayer, DataLayer)
{
	type.AddBaseObject<rsdn::layer::Layer>();

	TypeFactory::Regisiter(&type);
}
#pragma endregion

#pragma region rsdn::data::StringDerivedParameterPacket
__uregister_class1(rsdn::data::StringDerivedParameterPacket, StringDerivedParameterPacket)
{
	type.AddBaseObject<rsdn::data::ParameterPacket>();

	TypeFactory::Regisiter(&type);
}
#pragma endregion

#pragma region rsdn::data::SampleChunkLayer
__uregister_class1(rsdn::data::SampleChunkLayer, SampleChunkLayer)
{
	type.AddBaseObject<rsdn::layer::DataLayer>();

	TypeFactory::Regisiter(&type);
}
#pragma endregion

#pragma region rsdn::data::SampleChunkDataPacket
__uregister_class1(rsdn::data::SampleChunkDataPacket, SampleChunkDataPacket)
{
	type.AddBaseObject<rsdn::data::DataPacket>();

	TypeFactory::Regisiter(&type);
}
#pragma endregion