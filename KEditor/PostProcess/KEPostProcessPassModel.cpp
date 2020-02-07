#include "KEPostProcessPassModel.h"
#include "KEPostProcessData.h"
#include "KRender/Publish/KEnumString.h"
#include "KRender/Interface/IKPostProcess.h"

KEPostProcessPassModel::KEPostProcessPassModel(IKPostProcessPass* pass)
	: m_Pass(pass),
	m_Format(EF_R8GB8BA8_UNORM),
	m_Scale(1.0f),
	m_MSAA(1),
	m_VSFile("test"),
	m_FSFile("string")
{
	m_FormatView = KEditor::MakeComboEditView<ElementFormat>(&m_Format);
	m_ScaleView = KEditor::MakeLineEditView<float>(&m_Scale);
	m_MSAAView = KEditor::MakeSliderEditView<int>(&m_MSAA);
	m_VSView = KEditor::MakeLineEditView<std::string>(&m_VSFile);
	m_FSView = KEditor::MakeLineEditView<std::string>(&m_FSFile);

	m_MSAAView->SafeSliderCast<int>()->SetRange(1, 8);

	for (uint32_t f = 0; f < EF_COUNT; ++f)
	{
		m_FormatView->SafeComboCast<ElementFormat>()->AppendMapping(
			ElementFormat(f),
			KEnumString::ElementForamtToString(ElementFormat(f)));
	}

	m_FormatView->SafeCast<ElementFormat>()->SetValue({ EF_R16G16B16A16_FLOAT });
	m_ScaleView->SafeCast<float>()->SetValue({ 1.0f });
	m_MSAAView->SafeCast<int>()->SetValue({ 2 });

	QVBoxLayout* layout = new QVBoxLayout();

	{
		QHBoxLayout* lineLayout = new QHBoxLayout();
		QLabel* label = new QLabel("Format");
		lineLayout->addWidget(label);
		lineLayout->addLayout(m_FormatView->GetLayout());
		layout->addLayout(lineLayout);
	}

	{
		QHBoxLayout* lineLayout = new QHBoxLayout();
		QLabel* label = new QLabel("Scale");
		lineLayout->addWidget(label);
		lineLayout->addLayout(m_ScaleView->GetLayout());
		layout->addLayout(lineLayout);
	}

	{
		QHBoxLayout* lineLayout = new QHBoxLayout();
		QLabel* label = new QLabel("MSAA");
		lineLayout->addWidget(label);
		lineLayout->addLayout(m_MSAAView->GetLayout());
		layout->addLayout(lineLayout);
	}

	{
		QHBoxLayout* lineLayout = new QHBoxLayout();
		QLabel* label = new QLabel("VS");
		lineLayout->addWidget(label);
		lineLayout->addLayout(m_VSView->GetLayout());
		layout->addLayout(lineLayout);
	}

	{
		QHBoxLayout* lineLayout = new QHBoxLayout();
		QLabel* label = new QLabel("FS");
		lineLayout->addWidget(label);
		lineLayout->addLayout(m_FSView->GetLayout());
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