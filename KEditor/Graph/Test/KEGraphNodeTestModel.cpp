#include "KEGraphNodeTestModel.h"
#include "KRender/Publish/KEnumString.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>

KEGraphNodeTestModel::KEGraphNodeTestModel()
{
	QVBoxLayout* layout = new QVBoxLayout();

	{
		QHBoxLayout* lineLayout = new QHBoxLayout();
		QLabel* label = new QLabel("Format");
		QComboBox* combo = new QComboBox();

		lineLayout->addWidget(label);
		lineLayout->addWidget(combo);

		for (uint32_t f = 0; f < EF_COUNT; ++f)
		{
			combo->addItem(KEnumString::ElementForamtToString(ElementFormat(f)));
		}

		layout->addLayout(lineLayout);
	}

	{
		QHBoxLayout* lineLayout = new QHBoxLayout();
		QLabel* label = new QLabel("Scale");
		QLineEdit* lineEdit = new QLineEdit();

		QDoubleValidator* doubleValidator = new QDoubleValidator(0.0, 1.0, 2, lineEdit);
		doubleValidator->setNotation(QDoubleValidator::StandardNotation);

		lineEdit->setValidator(doubleValidator);

		lineLayout->addWidget(label);
		lineLayout->addWidget(lineEdit);

		layout->addLayout(lineLayout);
	}

	{
		QHBoxLayout* lineLayout = new QHBoxLayout();
		QLabel* label = new QLabel("MSAA");
		QLineEdit* lineEdit = new QLineEdit();

		QIntValidator* intValidator = new QIntValidator(1, 8, lineEdit);
		lineEdit->setValidator(intValidator);

		lineLayout->addWidget(label);
		lineLayout->addWidget(lineEdit);

		layout->addLayout(lineLayout);
	}

	{
		QHBoxLayout* lineLayout = new QHBoxLayout();
		QLabel* label = new QLabel("Shader");
		QLineEdit* lineEdit = new QLineEdit();

		lineLayout->addWidget(label);
		lineLayout->addWidget(lineEdit);

		layout->addLayout(lineLayout);
	}

	m_EditWidget.setLayout(layout);
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
	return &m_EditWidget;
}