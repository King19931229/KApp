#include "KUIOverlayBase.h"

#include "Interface/IKRenderDevice.h"
#include "Interface/IKPipeline.h"
#include "Interface/IKBuffer.h"
#include "Interface/IKTexture.h"
#include "Interface/IKSampler.h"

#include "imgui.h"

KUIOverlayBase::KUIOverlayBase()
	: m_bNeedUpdate(true),
	m_UIContext(nullptr)
{

}

KUIOverlayBase::~KUIOverlayBase()
{

}

void KUIOverlayBase::InitImgui()
{
	ASSERT_RESULT(m_UIContext == nullptr);
	// Init ImGui
	m_UIContext = ImGui::CreateContext();
	// Color scheme
	ImGuiStyle& style = ImGui::GetStyle();
	style.Colors[ImGuiCol_TitleBg] = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
	style.Colors[ImGuiCol_TitleBgActive] = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
	style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(1.0f, 0.0f, 0.0f, 0.1f);
	style.Colors[ImGuiCol_MenuBarBg] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
	style.Colors[ImGuiCol_Header] = ImVec4(0.8f, 0.0f, 0.0f, 0.4f);
	style.Colors[ImGuiCol_HeaderActive] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
	style.Colors[ImGuiCol_HeaderHovered] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
	style.Colors[ImGuiCol_FrameBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.8f);
	style.Colors[ImGuiCol_CheckMark] = ImVec4(1.0f, 0.0f, 0.0f, 0.8f);
	style.Colors[ImGuiCol_SliderGrab] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
	style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(1.0f, 0.0f, 0.0f, 0.8f);
	style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(1.0f, 1.0f, 1.0f, 0.1f);
	style.Colors[ImGuiCol_FrameBgActive] = ImVec4(1.0f, 1.0f, 1.0f, 0.2f);
	style.Colors[ImGuiCol_Button] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
	style.Colors[ImGuiCol_ButtonHovered] = ImVec4(1.0f, 0.0f, 0.0f, 0.6f);
	style.Colors[ImGuiCol_ButtonActive] = ImVec4(1.0f, 0.0f, 0.0f, 0.8f);
	// Dimensions
	ImGuiIO& io = ImGui::GetIO();
	io.FontGlobalScale = 1.0f;
}

void KUIOverlayBase::UnInitImgui()
{
	if(m_UIContext)
	{
		ImGui::DestroyContext((ImGuiContext*)m_UIContext);
		m_UIContext = nullptr;
	}
}

bool KUIOverlayBase::Init(IKRenderDevice* renderDevice, IKRenderTarget* renderTarget)
{
	ASSERT_RESULT(renderDevice != nullptr);
	ASSERT_RESULT(renderTarget != nullptr);
	ASSERT_RESULT(UnInit());

	renderDevice->CreateIndexBuffer(m_IndexBuffer);
	renderDevice->CreateVertexBuffer(m_VertexBuffer);

	renderDevice->CreateShader(m_VertexShader);
	renderDevice->CreateShader(m_FragmentShader);

	renderDevice->CreateTexture(m_FontTexture);
	renderDevice->CreateSampler(m_FontSampler);

	m_Constant.shaderTypes = ST_VERTEX;
	m_Constant.size = sizeof(m_PushConstBlock);

	renderDevice->CreatePipeline(m_Pipeline);

	InitImgui();
	PrepareResources();
	PreparePipeline(renderTarget);

	return true;
}

bool KUIOverlayBase::UnInit()
{
	UnInitImgui();

	if(m_IndexBuffer)
	{
		m_IndexBuffer->UnInit();
		m_IndexBuffer = nullptr;
	}

	if(m_VertexBuffer)
	{
		m_VertexBuffer->UnInit();
		m_VertexBuffer = nullptr;
	}

	if(m_VertexShader)
	{
		m_VertexShader->UnInit();
		m_VertexShader = nullptr;
	}

	if(m_FragmentShader)
	{
		m_FragmentShader->UnInit();
		m_FragmentShader = nullptr;
	}

	if(m_FontTexture)
	{
		m_FontTexture->UnInit();
		m_FontTexture = nullptr;
	}

	if(m_FontSampler)
	{
		m_FontSampler->UnInit();
		m_FontSampler = nullptr;
	}

	if(m_Pipeline)
	{
		m_Pipeline->UnInit();
		m_Pipeline = nullptr;
	}

	return true;
}

void KUIOverlayBase::PrepareResources()
{
	ASSERT_RESULT(m_FontTexture != nullptr);
	ASSERT_RESULT(m_FontSampler != nullptr);

	ImGuiIO& io = ImGui::GetIO();
	// Create font texture
	unsigned char* fontData;
	int texWidth = 0, texHeight = 0;
	io.Fonts->AddFontFromFileTTF("Fonts/Roboto-Medium.ttf", 16.0f);	
	io.Fonts->GetTexDataAsRGBA32(&fontData, &texWidth, &texHeight);

	ASSERT_RESULT(m_FontTexture->InitMemoryFromData(fontData, (size_t)texWidth, (size_t)texHeight, IF_R8G8B8A8, false));
	ASSERT_RESULT(m_FontTexture->InitDevice());

	m_FontSampler->SetAddressMode(AM_CLAMP_TO_BORDER, AM_CLAMP_TO_BORDER, AM_CLAMP_TO_BORDER);
	m_FontSampler->SetFilterMode(FM_LINEAR, FM_LINEAR);
	ASSERT_RESULT(m_FontSampler->Init());

	ASSERT_RESULT(m_VertexShader->InitFromFile("Shaders/uioverlay.vert"));
	ASSERT_RESULT(m_FragmentShader->InitFromFile("Shaders/uioverlay.frag"));
}

void KUIOverlayBase::PreparePipeline(IKRenderTarget* renderTarget)
{
	ASSERT_RESULT(m_Pipeline != nullptr);

	VertexFormat vertexFormats[] = {VF_GUI_POS_UV_COLOR};
	VertexInputDetail detail = { vertexFormats, ARRAY_SIZE(vertexFormats) };

	m_Pipeline->SetVertexBinding(&detail, 1);
	m_Pipeline->SetPrimitiveTopology(PT_TRIANGLE_LIST);
	m_Pipeline->SetBlendEnable(true);
	m_Pipeline->SetColorBlend(BF_SRC_COLOR, BF_ONE_MINUS_SRC_COLOR,	BO_ADD);
	m_Pipeline->SetCullMode(CM_NONE);
	m_Pipeline->SetFrontFace(FF_COUNTER_CLOCKWISE);
	m_Pipeline->SetPolygonMode(PM_FILL);
	m_Pipeline->SetShader(ST_VERTEX, m_VertexShader);
	m_Pipeline->SetShader(ST_FRAGMENT, m_FragmentShader);
	m_Pipeline->SetTextureSampler(0, m_FontTexture, m_FontSampler);
	m_Pipeline->PushConstantBlock(m_Constant, m_ConstantLoc);

	ASSERT_RESULT(m_Pipeline->Init(renderTarget));
}

bool KUIOverlayBase::Update()
{
	if(!m_bNeedUpdate)
		return true;

	ImDrawData* imDrawData = ImGui::GetDrawData();
	bool updateCmdBuffers = false;

	if (!imDrawData) 
	{
		return true;
	};

	if(imDrawData->TotalVtxCount == 0 || imDrawData->TotalIdxCount == 0)
	{
		return true;
	}

	size_t vertexBufferSize = imDrawData->TotalVtxCount * sizeof(ImDrawVert);
	size_t indexBufferSize = imDrawData->TotalIdxCount * sizeof(ImDrawIdx);

	if(vertexBufferSize > m_VertexBuffer->GetBufferSize())
	{
		m_VertexBuffer->UnInit();
		m_VertexBuffer->InitMemory(imDrawData->TotalVtxCount, sizeof(ImDrawVert), nullptr);
		m_VertexBuffer->InitDevice(true);
	}

	if(indexBufferSize > m_IndexBuffer->GetBufferSize())
	{
		m_IndexBuffer->UnInit();
		m_IndexBuffer->InitMemory(sizeof(ImDrawIdx) == 2 ? IT_16 : IT_32, imDrawData->TotalIdxCount, nullptr);
		m_IndexBuffer->InitDevice(true);
	}

	ImDrawVert* vtxDst = nullptr;
	ImDrawIdx* idxDst  = nullptr;

	ASSERT_RESULT(m_VertexBuffer->Map((void**)&vtxDst));
	ASSERT_RESULT(m_IndexBuffer->Map((void**)&idxDst));

	for (int n = 0; n < imDrawData->CmdListsCount; n++)
	{
		const ImDrawList* cmd_list = imDrawData->CmdLists[n];
		memcpy(vtxDst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
		memcpy(idxDst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
		vtxDst += cmd_list->VtxBuffer.Size;
		idxDst += cmd_list->IdxBuffer.Size;
	}

	ASSERT_RESULT(m_VertexBuffer->UnMap());
	ASSERT_RESULT(m_IndexBuffer->UnMap());

	m_bNeedUpdate = false;
	return true;
}

bool KUIOverlayBase::Resize(size_t width, size_t height)
{
	ImGuiIO& io = ImGui::GetIO();
	io.DisplaySize = ImVec2((float)(width), (float)(height));
	return true;
}

bool KUIOverlayBase::Begin(const char* str)
{
	ImGui::Begin(str, nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
	return true;
}

bool KUIOverlayBase::SetWindowPos(unsigned int x, unsigned int y)
{
	ImGui::SetNextWindowPos(ImVec2((float)x, (float)y));
	return true;
}

bool KUIOverlayBase::SetWindowSize(unsigned int width, unsigned int height)
{
	ImGui::SetNextWindowSize(ImVec2((float)width, (float)height), ImGuiCond_FirstUseEver);
	return true;
}

bool KUIOverlayBase::PushItemWidth(float width)
{
	ImGui::PushItemWidth(width);
	return true;
}

bool KUIOverlayBase::PopItemWidth()
{
	ImGui::PopItemWidth();
	return true;
}

bool KUIOverlayBase::End()
{
	ImGui::End();
	return true;
}

bool KUIOverlayBase::SetMousePosition(unsigned int x, unsigned int y)
{
	ImGuiIO& io = ImGui::GetIO();
	io.MousePos = ImVec2((float)x, (float)y);
	return true;
}

bool KUIOverlayBase::SetMouseDown(InputMouseButton button, bool down)
{
	ImGuiIO& io = ImGui::GetIO();
	switch (button)
	{
	case INPUT_MOUSE_BUTTON_LEFT:
		io.MouseDown[0] = down;
		break;
	case INPUT_MOUSE_BUTTON_RIGHT:
		io.MouseDown[1] = down;
		break;
	case INPUT_MOUSE_BUTTON_MIDDLE:
		io.MouseDown[2] = down;
		break;
	default:
		assert(false && "unknown button");
		return false;
	}
	return true;
}

bool KUIOverlayBase::StartNewFrame()
{
	ImGui::NewFrame();
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
	return true;
}

bool KUIOverlayBase::EndNewFrame()
{
	ImGui::PopStyleVar();
	ImGui::Render();
	return true;
}

bool KUIOverlayBase::Header(const char* caption)
{
	return ImGui::CollapsingHeader(caption, ImGuiTreeNodeFlags_DefaultOpen);
}

bool KUIOverlayBase::CheckBox(const char* caption, bool* value)
{
	bool res = ImGui::Checkbox(caption, value);
	if (res) { m_bNeedUpdate = true; };
	return res;
}

bool KUIOverlayBase::InputFloat(const char* caption, float* value, float step, unsigned int precision)
{
	bool res = ImGui::InputFloat(caption, value, step, step * 10.0f, precision);
	if (res) { m_bNeedUpdate = true; };
	return res;
}

bool KUIOverlayBase::SliderFloat(const char* caption, float* value, float min, float max)
{
	bool res = ImGui::SliderFloat(caption, value, min, max);
	if (res) { m_bNeedUpdate = true; };
	return res;
}

bool KUIOverlayBase::SliderInt(const char* caption, int* value, int min, int max)
{
	bool res = ImGui::SliderInt(caption, value, min, max);
	if (res) { m_bNeedUpdate = true; };
	return res;
}

bool KUIOverlayBase::ComboBox(const char* caption, int* itemindex, const std::vector<std::string>& items)
{
	if (items.empty())
	{
		return false;
	}
	std::vector<const char*> charitems;
	charitems.reserve(items.size());
	for (size_t i = 0; i < items.size(); i++)
	{
		charitems.push_back(items[i].c_str());
	}
	int itemCount = static_cast<int>(charitems.size());
	bool res = ImGui::Combo(caption, itemindex, &charitems[0], itemCount, itemCount);
	if (res) { m_bNeedUpdate = true; };
	return res;
}

bool KUIOverlayBase::Button(const char* caption)
{
	bool res = ImGui::Button(caption);
	if (res) { m_bNeedUpdate = true; };
	return res;
}

void KUIOverlayBase::Text(const char* formatstr, ...)
{
	va_list args;
	va_start(args, formatstr);
	ImGui::TextV(formatstr, args);
	va_end(args);
	m_bNeedUpdate = true;
}