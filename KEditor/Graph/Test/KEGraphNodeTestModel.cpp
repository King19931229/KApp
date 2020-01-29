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

QString KEGraphNodeTestModel::PortCaption(PortType type, PortIndexType index) const
{
	switch (type)
	{
	case PT_IN:
		return "Input";
	case PT_OUT:
		return "Output";
	};
	return "";
}

bool KEGraphNodeTestModel::PortCaptionVisible(PortType type, PortIndexType index) const
{
	return true;
}

unsigned int KEGraphNodeTestModel::NumPorts(PortType portType) const
{
	switch (portType)
	{
	case PT_IN:
		return 2;
	case PT_OUT:
		return 3;
	default:
		return 0;
	}
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