#include "KEPostProcessPassModel.h"
#include "KEPostProcessData.h"
#include "KRender/Publish/KEnumString.h"
#include "KRender/Interface/IKPostProcess.h"

#include "Property/KEPropertyModel.h"
#include "Property/KEPropertyViewModel.h"
#include "Property/KEPropertyRegularView.h"

KEPostProcessPassModel::KEPostProcessPassModel(IKPostProcessPass* pass)
	: m_Pass(pass)
{

	QVBoxLayout* layout = new QVBoxLayout();
	/*	
	{
		KEPropertyModel<float> *model = new KEPropertyModel<float>();
		KEPropertyRegularView<float> *view = new KEPropertyRegularView<float>();
		KEPropertyViewModel<float> *viewModel = new KEPropertyViewModel<float>(*model, *view);
		viewModel->SetValue({300.0f});
		layout->addLayout(view->GetLayout());
	}

	{
		KEPropertyModel<std::string> *model = new KEPropertyModel<std::string>();
		KEPropertyRegularView<std::string> *view = new KEPropertyRegularView<std::string>();
		KEPropertyViewModel<std::string> *viewModel = new KEPropertyViewModel<std::string>(*model, *view);
		viewModel->SetValue({ "a" });
		layout->addLayout(view->GetLayout());
	}
	
	{
		KEPropertyModel<std::string, 2> *model = new KEPropertyModel<std::string, 2>();
		KEPropertyRegularView<std::string, 2> *view = new KEPropertyRegularView<std::string, 2>();
		KEPropertyViewModel<std::string, 2> *viewModel = new KEPropertyViewModel<std::string, 2>(*model, *view);
		viewModel->SetValue({ "b", "c" });
		layout->addLayout(view->GetLayout());
	}
	*/

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
		m_FormatCombo = combo;
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
		m_ScaleEdit = lineEdit;
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
		m_MSAAEdit = lineEdit;
	}

	{
		QHBoxLayout* lineLayout = new QHBoxLayout();
		QLabel* label = new QLabel("VSShader");
		QLineEdit* lineEdit = new QLineEdit();

		lineLayout->addWidget(label);
		lineLayout->addWidget(lineEdit);

		layout->addLayout(lineLayout);
		m_VSEdit = lineEdit;
	}

	{
		QHBoxLayout* lineLayout = new QHBoxLayout();
		QLabel* label = new QLabel("FSShader");
		QLineEdit* lineEdit = new QLineEdit();

		lineLayout->addWidget(label);
		lineLayout->addWidget(lineEdit);

		layout->addLayout(lineLayout);
		m_FSEdit = lineEdit;
	}

	m_EditWidget = new QWidget();
	m_EditWidget->setLayout(layout);
}

KEPostProcessPassModel::~KEPostProcessPassModel()
{
	// assert(!m_Pass);
}

QString	KEPostProcessPassModel::Caption() const
{
	return "Pass";
}

QString	KEPostProcessPassModel::Name() const
{
	return "ProcessPass";
}

QString	KEPostProcessPassModel::PortCaption(PortType type, PortIndexType index) const
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

bool KEPostProcessPassModel::PortCaptionVisible(PortType type, PortIndexType index) const
{
	return true;
}

unsigned int KEPostProcessPassModel::NumPorts(PortType portType) const
{
	switch (portType)
	{
	case PT_IN:
		return MAX_INPUT_SLOT_COUNT;
	case PT_OUT:
		return MAX_OUTPUT_SLOT_COUNT;
	default:
		return 0;
	}
}

KEGraphNodeDataType KEPostProcessPassModel::DataType(PortType portType, PortIndexType portIndex) const
{
	return KEGraphNodeDataType();
}

void KEPostProcessPassModel::SetInData(KEGraphNodeDataPtr nodeData, PortIndexType port)
{
	//TODO
}

KEGraphNodeDataPtr KEPostProcessPassModel::OutData(PortIndexType port)
{
	KEPostProcessPassData* passData = new KEPostProcessPassData(m_Pass);
	KEGraphNodeDataPtr data = KEGraphNodeDataPtr(passData);
	return data;
}

QWidget* KEPostProcessPassModel::EmbeddedWidget()
{
	return m_EditWidget;
}