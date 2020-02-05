#pragma once
#include "Graph/Node/KEGraphNodeModel.h"

class KEGraphEmtpyNodeModel : public KEGraphNodeModel
{
	Q_OBJECT
protected:
	unsigned int m_InPorts;
	unsigned int m_OutPorts;
public:
	KEGraphEmtpyNodeModel(unsigned int inPorts, unsigned int outPorts);
	virtual	~KEGraphEmtpyNodeModel();

	virtual QString	Caption() const override;
	virtual QString	Name() const override;

	virtual QString	PortCaption(PortType type, PortIndexType index) const override;
	virtual bool PortCaptionVisible(PortType type, PortIndexType index) const override;

	virtual	unsigned int NumPorts(PortType portType) const override;
	virtual	KEGraphNodeDataType DataType(PortType portType, PortIndexType portIndex) const override;

	virtual	void SetInData(KEGraphNodeDataPtr nodeData, PortIndexType port) override;
	virtual KEGraphNodeDataPtr OutData(PortIndexType port) override;

	virtual	QWidget* EmbeddedWidget() override;
};