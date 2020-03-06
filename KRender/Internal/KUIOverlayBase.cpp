#include "KUIOverlayBase.h"

#include "Interface/IKRenderDevice.h"
#include "Interface/IKPipeline.h"
#include "Interface/IKBuffer.h"
#include "Interface/IKTexture.h"
#include "Interface/IKSampler.h"
#include "KBase/Interface/IKFileSystem.h"

#include "Internal/KRenderGlobal.h"

#include "imgui.h"

KUIOverlayBase::KUIOverlayBase()
	: m_UIContext(nullptr)
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

bool KUIOverlayBase::Init(IKRenderDevice* renderDevice, size_t frameInFlight)
{
	ASSERT_RESULT(renderDevice != nullptr);
	ASSERT_RESULT(frameInFlight > 0);
	ASSERT_RESULT(UnInit());

	renderDevice->CreateShader(m_VertexShader);
	renderDevice->CreateShader(m_FragmentShader);

	renderDevice->CreateTexture(m_FontTexture);
	renderDevice->CreateSampler(m_FontSampler);

	size_t numImages = frameInFlight;

	m_IndexBuffers.resize(numImages);
	m_VertexBuffers.resize(numImages);
	m_Pipelines.resize(numImages);
	m_NeedUpdates.resize(numImages);

	for(size_t i = 0; i < numImages; ++i)
	{
		renderDevice->CreateIndexBuffer(m_IndexBuffers[i]);
		renderDevice->CreateVertexBuffer(m_VertexBuffers[i]);
		KRenderGlobal::PipelineManager.CreatePipeline(m_Pipelines[i]);
		m_NeedUpdates[i] = true;
	}

	InitImgui();
	PrepareResources();
	PreparePipeline();

	return true;
}

bool KUIOverlayBase::UnInit()
{
	UnInitImgui();

	for(IKIndexBufferPtr indexBuffer : m_IndexBuffers)
	{
		indexBuffer->UnInit();
		indexBuffer = nullptr;
	}
	m_IndexBuffers.clear();

	for(IKVertexBufferPtr vertexBuffer : m_VertexBuffers)
	{
		vertexBuffer->UnInit();
		vertexBuffer = nullptr;
	}
	m_VertexBuffers.clear();

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

	for(IKPipelinePtr pipeline : m_Pipelines)
	{
		KRenderGlobal::PipelineManager.DestroyPipeline(pipeline);
		pipeline = nullptr;
	}
	m_Pipelines.clear();

	m_NeedUpdates.clear();

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

	IKDataStreamPtr ttfDataStream = nullptr;
	if(KFileSystem::Manager->Open("Fonts/Roboto-Medium.ttf", IT_MEMORY, ttfDataStream))
	{
		size_t ttfDataSize = ttfDataStream->GetSize();
		char* ttfData = new char[ttfDataSize];
		// https://github.com/ocornut/imgui/issues/1259
		if(ttfDataStream->Read(ttfData, ttfDataSize))
		{
			io.Fonts->AddFontFromMemoryTTF(ttfData, (int)ttfDataSize, 16.0f);
			io.Fonts->GetTexDataAsRGBA32(&fontData, &texWidth, &texHeight);
		}
		else
		{
			delete[] ttfData;
		}
	}

	ASSERT_RESULT(m_FontTexture->InitMemoryFromData(fontData, (size_t)texWidth, (size_t)texHeight, IF_R8G8B8A8, false, false));
	ASSERT_RESULT(m_FontTexture->InitDevice(false));

	m_FontSampler->SetAddressMode(AM_CLAMP_TO_BORDER, AM_CLAMP_TO_BORDER, AM_CLAMP_TO_BORDER);
	m_FontSampler->SetFilterMode(FM_LINEAR, FM_LINEAR);
	ASSERT_RESULT(m_FontSampler->Init(0, 0));

	ASSERT_RESULT(m_VertexShader->InitFromFile("Shaders/uioverlay.vert", false));
	ASSERT_RESULT(m_FragmentShader->InitFromFile("Shaders/uioverlay.frag", false));
}

void KUIOverlayBase::PreparePipeline()
{
	VertexFormat vertexFormats[] = {VF_GUI_POS_UV_COLOR};

	for(size_t i = 0; i < m_Pipelines.size(); ++i)
	{
		IKPipelinePtr pipeline = m_Pipelines[i];
		pipeline->SetVertexBinding(vertexFormats, 1);
		pipeline->SetPrimitiveTopology(PT_TRIANGLE_LIST);
		pipeline->SetBlendEnable(true);
		pipeline->SetColorBlend(BF_SRC_ALPHA, BF_ONE_MINUS_SRC_ALPHA, BO_ADD);
		pipeline->SetCullMode(CM_NONE);
		pipeline->SetFrontFace(FF_COUNTER_CLOCKWISE);
		pipeline->SetPolygonMode(PM_FILL);
		pipeline->SetDepthFunc(CF_ALWAYS, false, false);
		pipeline->SetShader(ST_VERTEX, m_VertexShader);
		pipeline->SetShader(ST_FRAGMENT, m_FragmentShader);
		pipeline->SetSampler(0, m_FontTexture, m_FontSampler);

		pipeline->CreateConstantBlock(ST_VERTEX, sizeof(m_PushConstBlock));
		ASSERT_RESULT(pipeline->Init(false));
	}	
}

bool KUIOverlayBase::Update(unsigned int imageIndex)
{
	if(imageIndex < m_NeedUpdates.size())
	{
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

		if(vertexBufferSize > m_VertexBuffers[imageIndex]->GetBufferSize())
		{
			m_VertexBuffers[imageIndex]->UnInit();
			m_VertexBuffers[imageIndex]->InitMemory(imDrawData->TotalVtxCount, sizeof(ImDrawVert), nullptr);
			m_VertexBuffers[imageIndex]->InitDevice(true);
		}

		if(indexBufferSize > m_IndexBuffers[imageIndex]->GetBufferSize())
		{
			m_IndexBuffers[imageIndex]->UnInit();
			m_IndexBuffers[imageIndex]->InitMemory(sizeof(ImDrawIdx) == 2 ? IT_16 : IT_32, imDrawData->TotalIdxCount, nullptr);
			m_IndexBuffers[imageIndex]->InitDevice(true);
		}

		ImDrawVert* vtxDst = nullptr;
		ImDrawIdx* idxDst  = nullptr;

		ASSERT_RESULT(m_VertexBuffers[imageIndex]->Map((void**)&vtxDst));
		ASSERT_RESULT(m_IndexBuffers[imageIndex]->Map((void**)&idxDst));

		for (int n = 0; n < imDrawData->CmdListsCount; n++)
		{
			const ImDrawList* cmd_list = imDrawData->CmdLists[n];
			memcpy(vtxDst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
			memcpy(idxDst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
			vtxDst += cmd_list->VtxBuffer.Size;
			idxDst += cmd_list->IdxBuffer.Size;
		}

		ASSERT_RESULT(m_VertexBuffers[imageIndex]->UnMap());
		ASSERT_RESULT(m_IndexBuffers[imageIndex]->UnMap());

		return true;
	}
	return false;
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

void KUIOverlayBase::RemindUpdate()
{
	for(size_t i = 0; i < m_NeedUpdates.size(); ++i)
	{
		m_NeedUpdates[i] = true;
	}
}

bool KUIOverlayBase::CheckBox(const char* caption, bool* value)
{
	bool res = ImGui::Checkbox(caption, value);
	if (res) { RemindUpdate(); };
	return res;
}

bool KUIOverlayBase::InputFloat(const char* caption, float* value, float step, unsigned int precision)
{
	bool res = ImGui::InputFloat(caption, value, step, step * 10.0f, precision);
	if (res) { RemindUpdate(); };
	return res;
}

bool KUIOverlayBase::SliderFloat(const char* caption, float* value, float min, float max)
{
	bool res = ImGui::SliderFloat(caption, value, min, max);
	if (res) { RemindUpdate(); };
	return res;
}

bool KUIOverlayBase::SliderInt(const char* caption, int* value, int min, int max)
{
	bool res = ImGui::SliderInt(caption, value, min, max);
	if (res) { RemindUpdate(); };
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
	if (res) { RemindUpdate(); };
	return res;
}

bool KUIOverlayBase::Button(const char* caption)
{
	bool res = ImGui::Button(caption);
	if (res) { RemindUpdate(); };
	return res;
}

void KUIOverlayBase::Text(const char* formatstr, ...)
{
	va_list args;
	va_start(args, formatstr);
	ImGui::TextV(formatstr, args);
	va_end(args);
	RemindUpdate();
}