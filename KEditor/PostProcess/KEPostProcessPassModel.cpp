#include "KEPostProcessPassModel.h"
#include "KEPostProcessData.h"
#include "KRender/Publish/KEnumString.h"
#include "KRender/Interface/IKPostProcess.h"

KEPostProcessPassModel::KEPostProcessPassModel(IKPostProcessPass* pass)
	: m_Pass(pass),
	m_Widget(nullptr)
{
	m_FormatView = KEditor::MakeComboEditView<ElementFormat>();
	m_ScaleView = KEditor::MakeLineEditView<float>(1.0f);
	m_MSAAView = KEditor::MakeSliderEditView<int>(2);
	m_VSView = KEditor::MakeLineEditView<std::string>("VS");
	m_FSView = KEditor::MakeLineEditView<std::string>("FS");
	m_TestView = KEditor::MakeCheckBoxView<bool>(false);

	m_MSAAView->SafeSliderCast<int>()->SetRange(1, 8);

	for (uint32_t f = 0; f < EF_COUNT; ++f)
	{
		m_FormatView->SafeComboCast<ElementFormat>()->AppendMapping(
			ElementFormat(f),
			KEnumString::ElementForamtToString(ElementFormat(f)));
	}

	m_FormatView->Cast<ElementFormat>()->SetValue(EF_R16G16B16A16_FLOAT);

	m_Widget = new KEPropertyWidget();
	m_Widget->Init();

	m_Widget->AppendItem("Test", m_TestView);
	m_Widget->AppendItem("Format", m_FormatView);
	m_Widget->AppendItem("Scale", m_ScaleView);
	m_Widget->AppendItem("MSAA", m_MSAAView);
	m_Widget->AppendItem("VS", m_VSView);
	m_Widget->AppendItem("FS", m_FSView);
}

KEPostProcessPassModel::~KEPostProcessPassModel()
{
	m_Widget->UnInit();
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
	return m_Widget;
}