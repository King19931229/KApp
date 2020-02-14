#include "KEditorConfig.h"
#include "KEPostProcessTextureModel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>

decltype(KEPostProcessTextureModel::ModelName) KEPostProcessTextureModel::ModelName = "PostProcessTextrue";

KEPostProcessTextureModel::KEPostProcessTextureModel()
{
	QVBoxLayout* layout = new QVBoxLayout();

	{
		QHBoxLayout* lineLayout = new QHBoxLayout();
		QLabel* label = new QLabel("Texture");
		QLineEdit* lineEdit = new QLineEdit();

		lineLayout->addWidget(label);
		lineLayout->addWidget(lineEdit);

		layout->addLayout(lineLayout);
	}

	m_EditWidget = new QWidget();
	m_EditWidget->setLayout(layout);
}

KEPostProcessTextureModel::~KEPostProcessTextureModel()
{
	// m_EditWidget ÓÉNodeView³ÖÓÐÉ¾³ý
}

QString	KEPostProcessTextureModel::Caption() const
{
	return "Texture";
}

QString	KEPostProcessTextureModel::PortCaption(PortType type, PortIndexType index) const
{
	switch (type)
	{
	case PT_IN:
		return "";
	case PT_OUT:
		return "Output";
	};
	return "";
}

bool KEPostProcessTextureModel::PortCaptionVisible(PortType type, PortIndexType index) const
{
	return true;
}

unsigned int KEPostProcessTextureModel::NumPorts(PortType portType) const
{
	switch (portType)
	{
	case PT_IN:
		return 0;
	case PT_OUT:
		return 1;
	default:
		return 0;
	}
}

KEGraphNodeDataType KEPostProcessTextureModel::DataType(PortType portType, PortIndexType portIndex) const
{
	return KEGraphNodeDataType();
}

void KEPostProcessTextureModel::SetInData(KEGraphNodeDataPtr nodeData, PortIndexType port)
{
	//TODO
}

KEGraphNodeDataPtr KEPostProcessTextureModel::OutData(PortIndexType port)
{
	//TODO
	return nullptr;
}

QWidget* KEPostProcessTextureModel::EmbeddedWidget()
{
	return m_EditWidget;
}