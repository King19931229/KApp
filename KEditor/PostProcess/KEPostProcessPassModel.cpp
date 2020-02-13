#include "KEPostProcessPassModel.h"
#include "KEPostProcessData.h"
#include "KRender/Publish/KEnumString.h"
#include "KRender/Interface/IKPostProcess.h"

KEPostProcessPassModel::KEPostProcessPassModel(IKPostProcessNodePtr pass)
	: m_Pass(pass),
	m_Widget(nullptr)
{
	assert(m_Pass != nullptr);

	m_ScaleView = KEditor::MakeLineEditView<float>(m_Pass->CastPass()->GetScale());
	m_ScaleView->Cast<float>()->AddListener([this](auto value)
	{
		m_Pass->CastPass()->SetScale(value);
	});

	m_MSAAView = KEditor::MakeSliderEditView<int>(m_Pass->CastPass()->GetMSAA());
	m_MSAAView->Cast<int>()->AddListener([this](auto value)
	{
		m_Pass->CastPass()->SetMSAA(value);
	});

	m_MSAAView->SafeSliderCast<int>()->SetRange(1, 8);

	m_ShaderView = KEditor::MakeLineEditView<std::string, 2>(
	{
		std::get<0>(m_Pass->CastPass()->GetShader()),
		std::get<1>(m_Pass->CastPass()->GetShader())
	});

	m_ShaderView->Cast<std::string, 2>()->AddListener([this](auto value)
	{
		m_Pass->CastPass()->SetShader(value[0].c_str(), value[1].c_str());
	});

	m_FormatView = KEditor::MakeComboEditView<ElementFormat>();
	for (uint32_t f = 0; f < EF_COUNT; ++f)
	{
		m_FormatView->SafeComboCast<ElementFormat>()->AppendMapping(
			ElementFormat(f),
			KEnumString::ElementForamtToString(ElementFormat(f)));
	}
	m_FormatView->Cast<ElementFormat>()->SetValue(m_Pass->CastPass()->GetFormat());
	m_FormatView->Cast<ElementFormat>()->AddListener([this](auto value)
	{
		m_Pass->CastPass()->SetFormat(value);
	});

	m_Widget = new KEPropertyWidget();
	m_Widget->Init();

	m_Widget->AppendItem("Format", m_FormatView);
	m_Widget->AppendItem("Scale", m_ScaleView);
	m_Widget->AppendItem("MSAA", m_MSAAView);
	m_Widget->AppendItem("Shader", m_ShaderView);
}

KEPostProcessPassModel::~KEPostProcessPassModel()
{
	m_Widget->UnInit();
	IKPostProcessManager* mgr = GetProcessManager();
	mgr->DeleteNode(m_Pass);
	m_Pass = nullptr;
}

bool KEPostProcessPassModel::Deletable() const
{
	IKPostProcessManager* mgr = GetProcessManager();
	return mgr->GetStartPointPass() != m_Pass;
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
	KEPostProcessPassData* passData = new KEPostProcessPassData(m_Pass->CastPass());
	KEGraphNodeDataPtr data = KEGraphNodeDataPtr(passData);
	return data;
}

QWidget* KEPostProcessPassModel::EmbeddedWidget()
{
	return m_Widget;
}