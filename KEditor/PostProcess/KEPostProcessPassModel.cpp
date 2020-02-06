#include "KEPostProcessPassModel.h"
#include "KEPostProcessData.h"
#include "KRender/Publish/KEnumString.h"
#include "KRender/Interface/IKPostProcess.h"

#include "Property/KEPropertyModel.h"
#include "Property/KEPropertyViewModel.h"
#include "Property/KEPropertyLineEditView.h"

KEPostProcessPassModel::KEPostProcessPassModel(IKPostProcessPass* pass)
	: m_Pass(pass),
	m_Scale(1.0f),
	m_MSAA(1),
	m_VSFile("test"),
	m_FSFile("string")
{
	{
		auto model = KEditor::MakePropertyModelPtr<float>(&m_Scale);
		auto view = KEditor::MakeLineEditViewPtr<float>();
		m_ScaleViewModel = KEditor::MakePropertyViewModelPtr<float>(model, view);
	}

	{
		auto model = KEditor::MakePropertyModelPtr<int>(&m_MSAA);
		auto view = KEditor::MakeLineEditViewPtr<int>();
		m_MSAAViewModel = KEditor::MakePropertyViewModelPtr<int>(model, view);
	}

	{
		auto model = KEditor::MakePropertyModelPtr<std::string>(&m_VSFile);
		auto view = KEditor::MakeLineEditViewPtr<std::string>();
		m_VSViewModel = KEditor::MakePropertyViewModelPtr<std::string>(model, view);
	}

	{
		auto model = KEditor::MakePropertyModelPtr<std::string>(&m_FSFile);
		auto view = KEditor::MakeLineEditViewPtr<std::string>();
		m_FSViewModel = KEditor::MakePropertyViewModelPtr<std::string>(model, view);
	}

	QVBoxLayout* layout = new QVBoxLayout();

	/*
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
	*/
	{
		QHBoxLayout* lineLayout = new QHBoxLayout();
		QLabel* label = new QLabel("Scale");
		lineLayout->addWidget(label);
		lineLayout->addLayout(m_ScaleViewModel->GetView()->GetLayout());
		layout->addLayout(lineLayout);
	}

	{
		QHBoxLayout* lineLayout = new QHBoxLayout();
		QLabel* label = new QLabel("MSAA");
		lineLayout->addWidget(label);
		lineLayout->addLayout(m_MSAAViewModel->GetView()->GetLayout());
		layout->addLayout(lineLayout);
	}

	{
		QHBoxLayout* lineLayout = new QHBoxLayout();
		QLabel* label = new QLabel("VS");
		lineLayout->addWidget(label);
		lineLayout->addLayout(m_VSViewModel->GetView()->GetLayout());
		layout->addLayout(lineLayout);
	}

	{
		QHBoxLayout* lineLayout = new QHBoxLayout();
		QLabel* label = new QLabel("FS");
		lineLayout->addWidget(label);
		lineLayout->addLayout(m_FSViewModel->GetView()->GetLayout());
		layout->addLayout(lineLayout);
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