#pragma once
#include "Graph/Node/KEGraphNodeModel.h"
#include "KRender/Interface/IKPostProcess.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>

#include "Property/KEPropertyComboView.h"
#include "Property/KEPropertyLineEditView.h"

class KEPostProcessPassModel : public KEGraphNodeModel
{
	Q_OBJECT
protected:
	ElementFormat m_Format;
	float m_Scale;
	int m_MSAA;
	std::string m_VSFile;
	std::string m_FSFile;

	KEPropertyComboView<ElementFormat>::BasePtr m_FormatView;
	KEPropertyLineEditView<float>::BasePtr m_ScaleView;
	KEPropertyLineEditView<int>::BasePtr m_MSAAView;
	KEPropertyLineEditView<std::string>::BasePtr m_VSView;
	KEPropertyLineEditView<std::string>::BasePtr m_FSView;

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