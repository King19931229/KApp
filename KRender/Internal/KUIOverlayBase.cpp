#include "KUIOverlayBase.h"

#include "Interface/IKRenderDevice.h"
#include "Interface/IKPipeline.h"
#include "Interface/IKTexture.h"
#include "Interface/IKSampler.h"
#include "Interface/IKRenderWindow.h"
#include "KBase/Interface/IKFileSystem.h"

#include "Internal/KRenderGlobal.h"

#include "imgui.h"
#include "imgui_internal.h"

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
	ImGui::SetCurrentContext((ImGuiContext*)m_UIContext);
	ImGuiIO& io = ImGui::GetIO();
#ifdef _WIN32
	if (KRenderGlobal::MainWindow) ImGui::GetMainViewport()->PlatformHandleRaw = KRenderGlobal::MainWindow->GetHWND();
#endif
	io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
	if (KRenderGlobal::InEditor)
	{
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		io.IniFilename = "keditor-imgui.ini";
		ImGui::StyleColorsDark();
		return;
	}
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

bool KUIOverlayBase::Init()
{
	UnInit();
	ImGuiContext* previousContext = ImGui::GetCurrentContext();

	KRenderGlobal::RenderDevice->CreateTexture(m_FontTexture);
	KRenderGlobal::RenderDevice->CreateSampler(m_FontSampler);

	KRenderGlobal::RenderDevice->CreatePipeline(m_Pipeline);

	InitImgui();
	PrepareResources();
	PreparePipeline();
	m_LastFrameTime = std::chrono::steady_clock::now();
	if (previousContext) ImGui::SetCurrentContext(previousContext);

	return true;
}

bool KUIOverlayBase::UnInit()
{
	m_Textures.clear();
	{
		std::lock_guard<std::mutex> lock(m_InputMutex);
		m_InputEvents.clear();
	}
	UnInitImgui();

	m_VertexShader.Release();

	m_FragmentShader.Release();

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

	ImGuiIO& io = ((ImGuiContext*)m_UIContext)->IO;

	// Create font texture
	unsigned char* fontData = nullptr;
	int texWidth = 0, texHeight = 0;

	IKDataStreamPtr ttfDataStream = nullptr;

	IKFileSystemPtr system = KFileSystem::Manager->GetFileSystem(FSD_RESOURCE);
	if (system && system->Open("Fonts/Roboto-Medium.ttf", IT_MEMORY, ttfDataStream))
	{
		size_t ttfDataSize = ttfDataStream->GetSize();
		char* ttfData = static_cast<char*>(IM_ALLOC(ttfDataSize));
		// https://github.com/ocornut/imgui/issues/1259
		if(ttfDataStream->Read(ttfData, ttfDataSize))
		{
			io.Fonts->AddFontFromMemoryTTF(ttfData, (int)ttfDataSize, 16.0f);
		}
		else
		{
			IM_FREE(ttfData);
		}
	}
	if (io.Fonts->Fonts.empty())
	{
		io.Fonts->AddFontDefault();
	}
#ifdef _WIN32
	ImFileHandle chineseFont = KRenderGlobal::InEditor ? ImFileOpen("C:/Windows/Fonts/msyh.ttc", "rb") : nullptr;
	if (chineseFont)
	{
		ImFileClose(chineseFont);
		ImFontConfig config;
		config.MergeMode = true;
		io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/msyh.ttc", 16.0f, &config, io.Fonts->GetGlyphRangesChineseFull());
	}
#endif
	io.Fonts->GetTexDataAsRGBA32(&fontData, &texWidth, &texHeight);
	io.Fonts->SetTexID(0);

	ASSERT_RESULT(m_FontTexture->InitMemoryFromData(fontData, "ImguiFont", (size_t)texWidth, (size_t)texHeight, 1, PF_R8G8B8A8_UNORM, false, false, false));
	ASSERT_RESULT(m_FontTexture->InitDevice(false));

	m_FontSampler->SetAddressMode(AM_CLAMP_TO_EDGE, AM_CLAMP_TO_EDGE, AM_CLAMP_TO_EDGE);
	m_FontSampler->SetFilterMode(FM_LINEAR, FM_LINEAR);
	ASSERT_RESULT(m_FontSampler->Init(0, 0));

	ASSERT_RESULT(KRenderGlobal::ShaderManager.Acquire(ST_VERTEX, "others/uioverlay.vert", m_VertexShader, KRenderGlobal::EnableAsyncShaderCompile));
	ASSERT_RESULT(KRenderGlobal::ShaderManager.Acquire(ST_FRAGMENT, "others/uioverlay.frag", m_FragmentShader, KRenderGlobal::EnableAsyncShaderCompile));
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
	pipeline->SetShader(ST_VERTEX, m_VertexShader.Get());
	pipeline->SetShader(ST_FRAGMENT, m_FragmentShader.Get());
	pipeline->SetSampler(SHADER_BINDING_TEXTURE0, m_FontTexture->GetFrameBuffer(), m_FontSampler);

	pipeline->CreateConstantBlock(ST_VERTEX, sizeof(m_PushConstBlock));
	ASSERT_RESULT(pipeline->Init());
}

bool KUIOverlayBase::Resize(uint32_t width, uint32_t height)
{
	ImGuiIO& io = ((ImGuiContext*)m_UIContext)->IO;
	io.DisplaySize = ImVec2((float)(width), (float)(height));
	return true;
}

bool KUIOverlayBase::SetMousePosition(unsigned int x, unsigned int y)
{
	QueueInput(InputEvent::MousePosition, 0, (float)x, (float)y);
	return true;
}

bool KUIOverlayBase::SetMouseDown(InputMouseButton button, bool down)
{
	int index = -1;
	switch (button)
	{
	case INPUT_MOUSE_BUTTON_LEFT:
		index = 0;
		break;
	case INPUT_MOUSE_BUTTON_RIGHT:
		index = 1;
		break;
	case INPUT_MOUSE_BUTTON_MIDDLE:
		index = 2;
		break;
	default:
		assert(false && "unknown button");
		return false;
	}
	QueueInput(InputEvent::MouseButton, index, down ? 1.0f : 0.0f, 0.0f);
	return true;
}

bool KUIOverlayBase::SetMouseScroll(float x, float y)
{
	QueueInput(InputEvent::MouseScroll, 0, x * 0.4f, y * 0.4f);
	return true;
}

void KUIOverlayBase::QueueInput(InputEvent::Type type, int key, float x, float y)
{
	std::lock_guard<std::mutex> lock(m_InputMutex);
	m_InputEvents.emplace_back(type, key, x, y);
}

bool KUIOverlayBase::SetKey(int key, bool down)
{
	QueueInput(InputEvent::Key, key, down ? 1.0f : 0.0f, 0.0f);
	return true;
}

bool KUIOverlayBase::SetCharacter(uint32_t character)
{
	QueueInput(InputEvent::Character, (int)character, 0.0f, 0.0f);
	return true;
}

bool KUIOverlayBase::SetFocus(bool focused)
{
	QueueInput(InputEvent::Focus, 0, focused ? 1.0f : 0.0f, 0.0f);
	return true;
}

uint64_t KUIOverlayBase::RegisterTexture(IKFrameBufferPtr image, IKSamplerPtr sampler)
{
	if (!image) return 0;
	uint64_t id = m_NextTexture++;
	m_Textures[id] = { image, sampler ? sampler : m_FontSampler };
	return id;
}

bool KUIOverlayBase::UnregisterTexture(uint64_t texture)
{
	return m_Textures.erase(texture) != 0;
}

bool KUIOverlayBase::StartNewFrame()
{
	ImGui::SetCurrentContext((ImGuiContext*)m_UIContext);
	ImGuiIO& io = ImGui::GetIO();
	std::vector<InputEvent> events;
	{
		std::lock_guard<std::mutex> lock(m_InputMutex);
		events.swap(m_InputEvents);
	}
	for (const InputEvent& event : events)
	{
		switch (event.type)
		{
		case InputEvent::MousePosition: io.AddMousePosEvent(event.x, event.y); break;
		case InputEvent::MouseButton: io.AddMouseButtonEvent(event.key, event.x != 0.0f); break;
		case InputEvent::MouseScroll: io.AddMouseWheelEvent(event.x, event.y); break;
		case InputEvent::Key: io.AddKeyEvent((ImGuiKey)event.key, event.x != 0.0f); break;
		case InputEvent::Character: io.AddInputCharacter((unsigned int)event.key); break;
		case InputEvent::Focus: io.AddFocusEvent(event.x != 0.0f); break;
		}
	}
	auto now = std::chrono::steady_clock::now();
	io.DeltaTime = std::max(0.000001f, std::chrono::duration<float>(now - m_LastFrameTime).count());
	m_LastFrameTime = now;
	ImGui::NewFrame();
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
	return true;
}

bool KUIOverlayBase::EndNewFrame()
{
	ImGui::SetCurrentContext((ImGuiContext*)m_UIContext);
	ImGui::PopStyleVar();
	ImGui::Render();
	return true;
}