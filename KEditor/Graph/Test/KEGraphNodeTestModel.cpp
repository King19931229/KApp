#include "KEGraphNodeTestModel.h"

KEGraphNodeTestModel::KEGraphNodeTestModel()
{
}

KEGraphNodeTestModel::~KEGraphNodeTestModel()
{
}

QString	KEGraphNodeTestModel::Caption() const
{
	return QString("TestCaption");
}

QString	KEGraphNodeTestModel::Name() const
{
	return QString("TestName");
}

unsigned int KEGraphNodeTestModel::NumPorts(PortType portType) const
{
	return 0;
}

KEGraphNodeDataType KEGraphNodeTestModel::DataType(PortType portType, uint32_t portIndex) const
{
	return KEGraphNodeDataType();
}