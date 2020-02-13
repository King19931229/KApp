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
#include "Property/KEPropertySliderView.h"
#include "Property/KEPropertyCheckBoxView.h"
#include "Widget/KEPropertyWidget.h"

class KEPostProcessPassModel : public KEGraphNodeModel
{
	Q_OBJECT
protected:
	IKPostProcessNodePtr m_Pass;

	KEPropertyWidget* m_Widget;

	KEPropertyComboView<ElementFormat>::BasePtr m_FormatView;
	KEPropertyLineEditView<float>::BasePtr m_ScaleView;
	KEPropertySliderView<int>::BasePtr m_MSAAView;
	KEPropertyLineEditView<std::string, 2>::BasePtr m_ShaderView;
public:
	KEPostProcessPassModel(IKPostProcessNodePtr pass = nullptr);
	virtual	~KEPostProcessPassModel();

	inline IKPostProcessPass* GetPass() { return m_Pass->CastPass(); }

	virtual bool Deletable() const override;

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