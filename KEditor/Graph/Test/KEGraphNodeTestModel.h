#pragma once
#include "Graph/Node/KEGraphNodeModel.h"

class KEGraphNodeTestModel : public KEGraphNodeModel
{
	Q_OBJECT
public:
	KEGraphNodeTestModel();
	virtual	~KEGraphNodeTestModel();

	/// Caption is used in GUI
	virtual QString	Caption() const override;
	virtual QString	Name() const override;

	virtual	unsigned int NumPorts(PortType portType) const override;
	virtual	KEGraphNodeDataType DataType(PortType portType, uint32_t portIndex) const override;
};