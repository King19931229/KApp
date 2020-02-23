#include "KEGraphEmtpyNodeModel.h"

KEGraphEmtpyNodeModel::KEGraphEmtpyNodeModel(unsigned int inPorts, unsigned int outPorts, bool redoable)
	: m_InPorts(inPorts),
	m_OutPorts(outPorts),
	m_bRedoable(redoable)
{
}

KEGraphEmtpyNodeModel::~KEGraphEmtpyNodeModel()
{

}

bool KEGraphEmtpyNodeModel::Redoable() const
{
	return m_bRedoable;
}

QString	KEGraphEmtpyNodeModel::Caption() const
{
	return "Empty";
}

QString	KEGraphEmtpyNodeModel::Name() const
{
	return "Empty";
}

QString	KEGraphEmtpyNodeModel::PortCaption(PortType type, PortIndexType index) const
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

bool KEGraphEmtpyNodeModel::PortCaptionVisible(PortType type, PortIndexType index) const
{
	return true;
}

unsigned int KEGraphEmtpyNodeModel::NumPorts(PortType portType) const
{
	switch (portType)
	{
	case PT_IN:
		return m_InPorts;
	case PT_OUT:
		return m_OutPorts;
	default:
		return 0;
	}
}

KEGraphNodeDataType KEGraphEmtpyNodeModel::DataType(PortType portType, PortIndexType portIndex) const
{
	return KEGraphNodeDataType();
}

void KEGraphEmtpyNodeModel::SetInData(KEGraphNodeDataPtr nodeData, PortIndexType port)
{
}

KEGraphNodeDataPtr KEGraphEmtpyNodeModel::OutData(PortIndexType port)
{
	return nullptr;
}

QWidget* KEGraphEmtpyNodeModel::EmbeddedWidget()
{
	return nullptr;
}