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

KEGraphNodeDataType KEGraphNodeTestModel::DataType(PortType portType, PortIndexType portIndex) const
{
	return KEGraphNodeDataType();
}

void KEGraphNodeTestModel::SetInData(KEGraphNodeDataPtr nodeData, PortIndexType port)
{
	return;
}

KEGraphNodeDataPtr KEGraphNodeTestModel::OutData(PortIndexType port)
{
	return nullptr;
}

QWidget* KEGraphNodeTestModel::EmbeddedWidget()
{
	return nullptr;
}