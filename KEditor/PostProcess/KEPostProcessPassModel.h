#pragma once
#include "Graph/Node/KEGraphNodeModel.h"
#include "KRender/Interface/IKPostProcess.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>

#include "Property/KEPropertyViewModel.h"

class KEPostProcessPassModel : public KEGraphNodeModel
{
	Q_OBJECT
protected:
	float m_Scale;
	int m_MSAA;
	std::string m_VSFile;
	std::string m_FSFile;

	KEPropertyViewModel<float>::PtrType m_ScaleViewModel;
	KEPropertyViewModel<int>::PtrType m_MSAAViewModel;
	KEPropertyViewModel<std::string>::PtrType m_VSViewModel;
	KEPropertyViewModel<std::string>::PtrType m_FSViewModel;

	QComboBox* m_FormatCombo;
	QWidget* m_EditWidget;

	IKPostProcessPass* m_Pass;
public:
	KEPostProcessPassModel(IKPostProcessPass* pass = nullptr);
	virtual	~KEPostProcessPassModel();

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