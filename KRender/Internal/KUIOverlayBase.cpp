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
	, m_PushOffset(0)
{
}

KUIOverlayBase::~KUIOverlayBase()
{
}

static uint32_t ImGuiColorEnum(UIOverlayColor target)
{
	switch (target)
	{
		case UI_COLOR_TEXT:
		{
			return ImGuiCol_Text;
		}
		case UI_COLOR_WINDOW_BACKGROUND:
		{
			return ImGuiCol_WindowBg;
		}
		case UI_COLOR_MENUBAR_BACKGROUND:
		{
			return ImGuiCol_MenuBarBg;
		}
		default:
		{
			assert(false && "unknown");
			return ImGuiCol_COUNT;
		}
	}
}

static ImColor ImGuiColor(KUIColor color)
{
	return ImColor(color.r, color.g, color.b, color.a);
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

	renderDevice->CreatePipeline(m_Pipeline);

	size_t numImages = frameInFlight;

	m_IndexBuffers.resize(numImages);
	m_VertexBuffers.resize(numImages);

	for(size_t i = 0; i < numImages; ++i)
	{
		renderDevice->CreateIndexBuffer(m_IndexBuffers[i]);
		renderDevice->CreateVertexBuffer(m_VertexBuffers[i]);
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

	SAFE_UNINIT(m_Pipeline);

	return true;
}

void KUIOverlayBase::PrepareResources()
{
	ASSERT_RESULT(m_FontTexture != nullptr);
	ASSERT_RESULT(m_FontSampler != nullptr);

	ImGuiIO& io = ImGui::GetIO();

	// Create font texture
	unsigned char* fontData = nullptr;
	int texWidth = 0, texHeight = 0;

	IKDataStreamPtr ttfDataStream = nullptr;

	IKFileSystemPtr system = KFileSystem::Manager->GetFileSystem(FSD_RESOURCE);
	if (system && system->Open("Fonts/Roboto-Medium.ttf", IT_MEMORY, ttfDataStream))
	{
		size_t ttfDataSize = ttfDataStream->GetSize();
		char* ttfData = KNEW char[ttfDataSize];
		// https://github.com/ocornut/imgui/issues/1259
		if(ttfDataStream->Read(ttfData, ttfDataSize))
		{
			io.Fonts->AddFontFromMemoryTTF(ttfData, (int)ttfDataSize, 16.0f);
			io.Fonts->GetTexDataAsRGBA32(&fontData, &texWidth, &texHeight);
		}
		else
		{
			KDELETE[] ttfData;
		}
	}

	ASSERT_RESULT(m_FontTexture->InitMemoryFromData(fontData, "ImguiFont", (size_t)texWidth, (size_t)texHeight, 1, IF_R8G8B8A8, false, false, false));
	ASSERT_RESULT(m_FontTexture->InitDevice(false));

	m_FontSampler->SetAddressMode(AM_CLAMP_TO_EDGE, AM_CLAMP_TO_EDGE, AM_CLAMP_TO_EDGE);
	m_FontSampler->SetFilterMode(FM_LINEAR, FM_LINEAR);
	ASSERT_RESULT(m_FontSampler->Init(0, 0));

	ASSERT_RESULT(m_VertexShader->InitFromFile(ST_VERTEX, "others/uioverlay.vert", false));
	ASSERT_RESULT(m_FragmentShader->InitFromFile(ST_FRAGMENT, "others/uioverlay.frag", false));
}

void KUIOverlayBase::PreparePipeline()
{
	VertexFormat vertexFormats[] = { VF_GUI_POS_UV_COLOR };

	IKPipelinePtr& pipeline = m_Pipeline;
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
	pipeline->SetSampler(SHADER_BINDING_TEXTURE0, m_FontTexture->GetFrameBuffer(), m_FontSampler);

	pipeline->CreateConstantBlock(ST_VERTEX, sizeof(m_PushConstBlock));
	ASSERT_RESULT(pipeline->Init());
}

bool KUIOverlayBase::Update()
{
	unsigned int imageIndex = KRenderGlobal::CurrentFrameIndex;
	
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
	for (int n = 0; n < imDrawData->CmdListsCount; n++)
	{
		const ImDrawList* cmd_list = imDrawData->CmdLists[n];
		memcpy(vtxDst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
		vtxDst += cmd_list->VtxBuffer.Size;
	}
	ASSERT_RESULT(m_VertexBuffers[imageIndex]->UnMap());

	ASSERT_RESULT(m_IndexBuffers[imageIndex]->Map((void**)&idxDst));
	for (int n = 0; n < imDrawData->CmdListsCount; n++)
	{
		const ImDrawList* cmd_list = imDrawData->CmdLists[n];
		memcpy(idxDst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
		idxDst += cmd_list->IdxBuffer.Size;
	}
	ASSERT_RESULT(m_IndexBuffers[imageIndex]->UnMap());

	return true;
}

bool KUIOverlayBase::Resize(size_t width, size_t height)
{
	ImGuiIO& io = ImGui::GetIO();
	io.DisplaySize = ImVec2((float)(width), (float)(height));
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

bool KUIOverlayBase::SetMouseScroll(float x, float y)
{
	ImGuiIO& io = ImGui::GetIO();
	float speed = 0.4f;
	io.MouseWheel += speed * y;
	io.MouseWheelH += speed * x;
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