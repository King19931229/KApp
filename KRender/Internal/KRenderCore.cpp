#include "KRenderCore.h"

#include "Interface/IKRenderWindow.h"
#include "Interface/IKRenderDevice.h"

#include "Internal/KRenderGlobal.h"

#include "KBase/Interface/IKAssetLoader.h"
#include "KBase/Interface/IKCodec.h"
#include "KBase/Publish/KPlatform.h"
#include "KBase/Publish/KTimer.h"

#include "Dispatcher/KRenderDispatcher.h"

#include "Internal/KConstantGlobal.h"

EXPORT_DLL IKRenderCorePtr CreateRenderCore()
{
	return IKRenderCorePtr(new KRenderCore());
}

KRenderCore::KRenderCore()
	: m_bInit(false),
	m_Device(nullptr),
	m_Window(nullptr),
	m_DebugConsole(nullptr),
	m_Gizmo(nullptr),
	m_MultiThreadSumbit(true),
	m_OctreeDebugDraw(false),
	m_MouseCtrlCamera(true)
{
}

KRenderCore::~KRenderCore()
{
	assert(!m_DebugConsole);
}

bool KRenderCore::InitRenderDispatcher()
{
	uint32_t frameInFlight = m_Device->GetFrameInFlight();
	IKSwapChainPtr swapChain = m_Device->GetCurrentSwapChain();
	IKUIOverlayPtr ui = m_Device->GetCurrentUIOverlay();

	KRenderGlobal::RenderDispatcher.Init(m_Device, frameInFlight, swapChain, ui);
	return true;
}

bool KRenderCore::UnInitRenderDispatcher()
{
	KRenderGlobal::RenderDispatcher.UnInit();
	return true;
}

bool KRenderCore::InitController()
{
	IKUIOverlayPtr ui = m_Device->GetCurrentUIOverlay();
	m_CameraMoveController.Init(&m_Camera, m_Window);
	m_UIController.Init(ui, m_Window);
	m_GizmoContoller.Init(m_Gizmo, &m_Camera, m_Window);

	m_KeyCallback = [this](InputKeyboard key, InputAction action)
	{
		if (key == INPUT_KEY_ENTER)
		{
			m_Device->Wait();
			KRenderGlobal::ShaderManager.Reload();
			KRenderGlobal::PipelineManager.Reload();
		}
	};

#if defined(_WIN32)
	m_Window->RegisterKeyboardCallback(&m_KeyCallback);
#endif
	return true;
}

bool KRenderCore::UnInitController()
{
	m_CameraMoveController.UnInit();
	m_UIController.UnInit();
	m_GizmoContoller.UnInit();
	return true;
}

bool KRenderCore::InitGizmo()
{
	m_Gizmo = CreateGizmo();
	m_Gizmo->Init(&m_Camera);
	m_Gizmo->SetManipulateMode(GizmoManipulateMode::GIZMO_MANIPULATE_LOCAL);
	m_Gizmo->SetMatrix(glm::rotate(glm::mat4(1.0f), glm::pi<float>() * 0.3f, glm::vec3(1.0f, 0.0f, 0.0f)) *
		glm::translate(glm::mat4(1.0f), glm::vec3(30.0f, 30.0f, 0.0f)));
	m_Gizmo->SetType(GizmoType::GIZMO_TYPE_ROTATE);
	m_Gizmo->Enter();
	return true;
}

bool KRenderCore::UnInitGizmo()
{
	m_Gizmo->UnInit();
	m_Gizmo = nullptr;
	return true;
}

bool KRenderCore::Init(IKRenderDevicePtr& device, IKRenderWindowPtr& window)
{
	if (!m_bInit)
	{
		m_Device = device.get();
		m_Window = window.get();
		m_DebugConsole = new KDebugConsole();
		m_DebugConsole->Init();

		m_Camera.SetPosition(glm::vec3(0, 400.0f, 400.0f));
		m_Camera.LookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		m_Camera.SetCustomLockYAxis(glm::vec3(0, 1, 0));
		m_Camera.SetLockYEnable(true);

		InitRenderDispatcher();		
		InitGizmo();
		InitController();

		m_PresentCallback = [this](uint32_t chainIndex, uint32_t frameIndex)
		{
			OnPresent(chainIndex, frameIndex);
		};

		m_Device->RegisterPresentCallback(&m_PresentCallback);
		
		m_bInit = true;
		return true;
	}
	return false;
}

bool KRenderCore::UnInit()
{
	if(m_bInit)
	{
		m_DebugConsole->UnInit();
		SAFE_DELETE(m_DebugConsole);

		UnInitRenderDispatcher();
		UnInitGizmo();
		UnInitController();

		m_Device->UnRegisterPresentCallback(&m_PresentCallback);

		m_Window = nullptr;
		m_Device = nullptr;

		m_bInit = false;
	}
	return true;
}

bool KRenderCore::Loop()
{
	if(m_bInit)
	{
		m_Window->Loop();
		return true;
	}
	return false;
}

bool KRenderCore::Tick()
{
	if (m_bInit)
	{
		m_Device->Present();
		return true;
	}
	return false;
}

bool KRenderCore::UpdateCamera(size_t frameIndex)
{
	IKUniformBufferPtr cameraBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(frameIndex, CBT_CAMERA);
	if (cameraBuffer)
	{
		glm::mat4 view = m_Camera.GetViewMatrix();
		glm::mat4 proj = m_Camera.GetProjectiveMatrix();
		glm::mat4 viewInv = glm::inverse(view);

		void* pWritePos = nullptr;
		void* pData = KConstantGlobal::GetGlobalConstantData(CBT_CAMERA);
		const KConstantDefinition::ConstantBufferDetail &details = KConstantDefinition::GetConstantBufferDetail(CBT_CAMERA);
		for (KConstantDefinition::ConstantSemanticDetail detail : details.semanticDetails)
		{
			if (detail.semantic == CS_VIEW)
			{
				pWritePos = POINTER_OFFSET(pData, detail.offset);
				assert(sizeof(view) == detail.size);
				memcpy(pWritePos, &view, sizeof(view));
			}
			else if (detail.semantic == CS_PROJ)
			{
				pWritePos = POINTER_OFFSET(pData, detail.offset);
				assert(sizeof(proj) == detail.size);
				memcpy(pWritePos, &proj, sizeof(proj));
			}
			else if (detail.semantic == CS_VIEW_INV)
			{
				pWritePos = POINTER_OFFSET(pData, detail.offset);
				assert(sizeof(viewInv) == detail.size);
				memcpy(pWritePos, &viewInv, sizeof(viewInv));
			}
		}
		cameraBuffer->Write(pData);
		return true;
	}
	return false;
}

static int numFrames = 0;
static float fps = 0.0f;
static float frameTime = 0.0f;
static int numFramesTotal = 0;
static float maxFrameTime = 0;
static float minFrameTime = 0;
static KTimer FPSTimer;
static KTimer MaxMinTimer;

bool KRenderCore::UpdateFrameTime()
{
	if (MaxMinTimer.GetMilliseconds() > 5000.0f)
	{
		maxFrameTime = frameTime;
		minFrameTime = frameTime;
		MaxMinTimer.Reset();
	}

	++numFrames;
	if (FPSTimer.GetMilliseconds() > 500.0f)
	{
		float time = FPSTimer.GetMilliseconds();
		time = time / (float)numFrames;
		frameTime = time;

		fps = 1000.0f / frameTime;

		if (frameTime > maxFrameTime)
		{
			maxFrameTime = time;
		}
		if (frameTime < minFrameTime)
		{
			minFrameTime = time;
		}
		FPSTimer.Reset();
		numFrames = 0;
	}

	char szBuffer[1024] = {};
	sprintf(szBuffer, "[FPS] %f [FrameTime] %f [MinTime] %f [MaxTime] %f [Frame]%d", fps, frameTime, minFrameTime, maxFrameTime, numFramesTotal++);
	m_Window->SetWindowTitle(szBuffer);

	return true;
}

bool KRenderCore::UpdateUIOverlay(size_t frameIndex)
{
	IKUIOverlayPtr ui = m_Device->GetCurrentUIOverlay();

	ui->StartNewFrame();
	{
		ui->SetWindowPos(10, 10);
		ui->SetWindowSize(0, 0);
		ui->Begin("Example");
		{
			ui->Text("FPS [%f] FrameTime [%f]", fps, frameTime);
			ui->PushItemWidth(110.0f);
			if (ui->Header("Setting"))
			{
				ui->CheckBox("MouseCtrlCamera", &m_MouseCtrlCamera);
				ui->CheckBox("OctreeDraw", &m_OctreeDebugDraw);
				ui->CheckBox("MultiRender", &m_MultiThreadSumbit);
				ui->SliderFloat("Shadow DepthBiasConstant", &KRenderGlobal::ShadowMap.GetDepthBiasConstant(), 0.0f, 5.0f);
				ui->SliderFloat("Shadow DepthBiasSlope", &KRenderGlobal::ShadowMap.GetDepthBiasSlope(), 0.0f, 5.0f);
			}
			ui->PopItemWidth();
		}
		ui->End();
	}
	ui->EndNewFrame();

	ui->Update((uint32_t)frameIndex);
	return true;
}

bool KRenderCore::UpdateController()
{
	static KTimer m_MoveTimer;

	const float dt = m_MoveTimer.GetSeconds();
	m_MoveTimer.Reset();

	m_CameraMoveController.Update(dt);
	return true;
}

bool KRenderCore::UpdateGizmo()
{
	m_Gizmo->Update();
	return true;
}

void KRenderCore::OnPresent(uint32_t chainIndex, uint32_t frameIndex)
{
	KRenderGlobal::TaskExecutor.ProcessSyncTask();

	UpdateFrameTime();
	UpdateCamera(frameIndex);
	UpdateUIOverlay(frameIndex);
	UpdateController();
	UpdateGizmo();

	KRenderGlobal::Scene.EnableDebugRender(m_OctreeDebugDraw);
	KRenderGlobal::RenderDispatcher.SetMultiThreadSumbit(m_MultiThreadSumbit);
	m_CameraMoveController.SetEnable(m_MouseCtrlCamera);

	KRenderGlobal::RenderDispatcher.Execute(&KRenderGlobal::Scene, &m_Camera, chainIndex, frameIndex);
}