#include "KRenderCore.h"

#include "Interface/IKRenderWindow.h"
#include "Interface/IKRenderDevice.h"

#include "Internal/KRenderGlobal.h"

#include "KBase/Interface/IKAssetLoader.h"
#include "KBase/Interface/IKCodec.h"
#include "KBase/Interface/Entity/IKEntityManager.h"
#include "KBase/Publish/KPlatform.h"
#include "KBase/Publish/KTimer.h"

#include "Dispatcher/KRenderDispatcher.h"

#include "Internal/KConstantGlobal.h"
#include "Internal/ECS/KECSGlobal.h"

#ifdef _WIN32
#	pragma warning(disable:4996)
#endif

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

bool KRenderCore::InitPostProcess()
{
	size_t width = 0, height = 0;
	m_Window->GetSize(width, height);

	uint32_t frameInFlight = m_Device->GetNumFramesInFlight();

	KRenderGlobal::PostProcessManager.Init(m_Device, width, height, 2, EF_R16G16B16A16_FLOAT, frameInFlight);
#if 0
	auto startPoint = KRenderGlobal::PostProcessManager.GetStartPointPass();

	auto pass = KRenderGlobal::PostProcessManager.CreatePass();
	pass->CastPass()->SetShader("Shaders/screenquad.vert", "Shaders/postprocess.frag");
	pass->CastPass()->SetScale(1.0f);
	pass->CastPass()->SetFormat(EF_R8GB8BA8_UNORM);

	auto pass2 = KRenderGlobal::PostProcessManager.CreatePass();
	pass2->CastPass()->SetShader("Shaders/screenquad.vert", "Shaders/postprocess2.frag");
	pass2->CastPass()->SetScale(1.0f);
	pass2->CastPass()->SetFormat(EF_R8GB8BA8_UNORM);

	auto pass3 = KRenderGlobal::PostProcessManager.CreatePass();
	pass3->CastPass()->SetShader("Shaders/screenquad.vert", "Shaders/postprocess3.frag");
	pass3->CastPass()->SetScale(1.0f);
	pass3->CastPass()->SetFormat(EF_R8GB8BA8_UNORM);

	KRenderGlobal::PostProcessManager.CreateConnection(startPoint, 0, pass, 0);
	KRenderGlobal::PostProcessManager.CreateConnection(startPoint, 0, pass2, 0);
	KRenderGlobal::PostProcessManager.CreateConnection(pass, 0, pass3, 0);
	KRenderGlobal::PostProcessManager.CreateConnection(pass2, 0, pass3, 1);

#ifdef _WIN32
	KRenderGlobal::PostProcessManager.Save("postprocess.json");
	KRenderGlobal::PostProcessManager.Load("postprocess.json");
#endif

#endif
	KRenderGlobal::PostProcessManager.Construct();
	return true;
}

bool KRenderCore::UnInitPostProcess()
{
	KRenderGlobal::PostProcessManager.UnInit();
	return true;
}

bool KRenderCore::InitGlobalManager()
{
	uint32_t frameInFlight = m_Device->GetNumFramesInFlight();

	KRenderGlobal::PipelineManager.Init(m_Device);
	KRenderGlobal::FrameResourceManager.Init(m_Device, frameInFlight);
	KRenderGlobal::MeshManager.Init(m_Device, frameInFlight);
	KRenderGlobal::ShaderManager.Init(m_Device);
	KRenderGlobal::TextrueManager.Init(m_Device);

	KRenderGlobal::SkyBox.Init(m_Device, frameInFlight, "Textures/uffizi_cube.ktx");
	KRenderGlobal::ShadowMap.Init(m_Device, frameInFlight, 2048);
	KRenderGlobal::CascadedShadowMap.Init(m_Device, frameInFlight, 3, 2048);

	return true;
}

bool KRenderCore::UnInitGlobalManager()
{
	KRenderGlobal::SkyBox.UnInit();
	KRenderGlobal::ShadowMap.UnInit();
	KRenderGlobal::CascadedShadowMap.UnInit();

	KRenderGlobal::MeshManager.UnInit();

	KRenderGlobal::TextrueManager.UnInit();
	KRenderGlobal::ShaderManager.UnInit();
	KRenderGlobal::PipelineManager.UnInit();

	KRenderGlobal::FrameResourceManager.UnInit();

	return true;
}

bool KRenderCore::InitRenderDispatcher()
{
	uint32_t frameInFlight = m_Device->GetNumFramesInFlight();
	IKSwapChainPtr swapChain = m_Device->GetSwapChain();
	IKUIOverlayPtr ui = m_Device->GetUIOverlay();

	m_CameraCube = CreateCameraCube();
	m_CameraCube->Init(m_Device, frameInFlight, &m_Camera);

	KRenderGlobal::RenderDispatcher.Init(m_Device, frameInFlight, swapChain, ui, m_CameraCube);
	return true;
}

bool KRenderCore::UnInitRenderDispatcher()
{
	m_CameraCube->UnInit();
	m_CameraCube = nullptr;

	KRenderGlobal::RenderDispatcher.UnInit();
	return true;
}

bool KRenderCore::InitController()
{
	IKUIOverlayPtr ui = m_Device->GetUIOverlay();
	m_CameraMoveController.Init(&m_Camera, m_Window, m_Gizmo);
	m_UIController.Init(ui, m_Window);
	m_GizmoContoller.Init(m_Gizmo, m_CameraCube, &m_Camera, m_Window);

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
	m_Gizmo->SetType(GizmoType::GIZMO_TYPE_MOVE);

	return true;
}

bool KRenderCore::UnInitGizmo()
{
	m_Gizmo->UnInit();
	m_Gizmo = nullptr;
	return true;
}

bool KRenderCore::InitRenderResource()
{
	if (KECS::EntityManager)
	{
		KECS::EntityManager->ViewAllEntity([](IKEntityPtr entity)
		{
			KRenderComponent* component = nullptr;
			if (entity->GetComponent(CT_RENDER, (IKComponentBase**)&component))
			{
				component->Init();
			}
		});
	}

	return true;
}

bool KRenderCore::UnInitRenderResource()
{
	if (KECS::EntityManager)
	{
		KECS::EntityManager->ViewAllEntity([](IKEntityPtr entity)
		{
			KRenderComponent* component = nullptr;
			if (entity->GetComponent(CT_RENDER, (IKComponentBase**)&component))
			{
				component->UnInit();
			}
		});
	}

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

		m_Camera.SetNear(1.0f);
		m_Camera.SetFar(5000.0f);
		m_Camera.SetCustomLockYAxis(glm::vec3(0, 1, 0));
		m_Camera.SetLockYEnable(true);

		m_PresentCallback = [this](uint32_t chainIndex, uint32_t frameIndex)
		{
			OnPresent(chainIndex, frameIndex);
		};

		m_SwapChainCallback = [this](uint32_t width, uint32_t height)
		{
			OnSwapChainRecreate(width, height);
		};

		m_InitCallback = [this]()
		{
			KRenderGlobal::TaskExecutor.Init(std::thread::hardware_concurrency());
			InitGlobalManager();
			InitPostProcess();
			InitRenderDispatcher();
			InitRenderResource();
			InitGizmo();
			InitController();

			for (KRenderCoreInitCallback* callback : m_Callbacks)
			{
				(*callback)();
			}
		};

		m_UnitCallback = [this]()
		{
			while (!KRenderGlobal::TaskExecutor.AllTaskDone())
			{
				KRenderGlobal::TaskExecutor.ProcessSyncTask();
			}
			assert(KRenderGlobal::TaskExecutor.AllTaskDone());

			KRenderGlobal::TaskExecutor.UnInit();

			UnInitController();
			UnInitGizmo();
			UnInitRenderResource();
			UnInitRenderDispatcher();
			UnInitPostProcess();
			UnInitGlobalManager();
		};

		KECSGlobal::Init();
		KRenderGlobal::Scene.Init(SCENE_MANGER_TYPE_OCTREE, 10000000.0f, glm::vec3(0.0f));

		m_Device->RegisterPresentCallback(&m_PresentCallback);
		m_Device->RegisterSwapChainRecreateCallback(&m_SwapChainCallback);
		m_Device->RegisterDeviceInitCallback(&m_InitCallback);
		m_Device->RegisterDeviceUnInitCallback(&m_UnitCallback);

		m_bInit = true;
		return true;
	}
	return false;
}

bool KRenderCore::UnInit()
{
	if (m_bInit)
	{
		KRenderGlobal::Scene.UnInit();
		KECSGlobal::UnInit();

		m_DebugConsole->UnInit();
		SAFE_DELETE(m_DebugConsole);

		m_Device->UnRegisterPresentCallback(&m_PresentCallback);
		m_Device->UnRegisterSwapChainRecreateCallback(&m_SwapChainCallback);
		m_Device->UnRegisterDeviceInitCallback(&m_InitCallback);
		m_Device->UnRegisterDeviceUnInitCallback(&m_UnitCallback);

		m_Window = nullptr;
		m_Device = nullptr;

		m_bInit = false;

		m_Callbacks.clear();
	}
	return true;
}

bool KRenderCore::Loop()
{
	if (m_bInit)
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

bool KRenderCore::Wait()
{
	if (m_bInit)
	{
		m_Device->Wait();
		return true;
	}
	return false;
}

bool KRenderCore::RegisterInitCallback(KRenderCoreInitCallback* callback)
{
	if (callback)
	{
		m_Callbacks.insert(callback);
		return true;
	}
	return false;
}

bool KRenderCore::UnRegisterInitCallback(KRenderCoreInitCallback* callback)
{
	if (callback)
	{
		auto it = m_Callbacks.find(callback);
		if (it != m_Callbacks.end())
		{
			m_Callbacks.erase(it);
			return true;
		}
	}
	return false;
}

bool KRenderCore::UnRegistertAllInitCallback()
{
	m_Callbacks.clear();
	return true;
}

IKRenderScene* KRenderCore::GetRenderScene()
{
	return &KRenderGlobal::Scene;
}

bool KRenderCore::UpdateCamera(size_t frameIndex)
{
	IKUniformBufferPtr cameraBuffer = KRenderGlobal::FrameResourceManager.GetConstantBuffer(frameIndex, CBT_CAMERA);
	if (cameraBuffer)
	{
		glm::mat4 view = m_Camera.GetViewMatrix();
		glm::mat4 proj = m_Camera.GetProjectiveMatrix();
		glm::mat4 viewInv = glm::inverse(view);

		void* pData = KConstantGlobal::GetGlobalConstantData(CBT_CAMERA);
		const KConstantDefinition::ConstantBufferDetail &details = KConstantDefinition::GetConstantBufferDetail(CBT_CAMERA);
		for (KConstantDefinition::ConstantSemanticDetail detail : details.semanticDetails)
		{
			void* pWritePos = nullptr;
			if (detail.semantic == CS_VIEW)
			{
				assert(sizeof(view) == detail.size);
				pWritePos = POINTER_OFFSET(pData, detail.offset);
				memcpy(pWritePos, &view, sizeof(view));
			}
			else if (detail.semantic == CS_PROJ)
			{
				assert(sizeof(proj) == detail.size);
				pWritePos = POINTER_OFFSET(pData, detail.offset);
				memcpy(pWritePos, &proj, sizeof(proj));
			}
			else if (detail.semantic == CS_VIEW_INV)
			{
				assert(sizeof(viewInv) == detail.size);
				pWritePos = POINTER_OFFSET(pData, detail.offset);
				memcpy(pWritePos, &viewInv, sizeof(viewInv));
			}
		}
		cameraBuffer->Write(pData);
		return true;
	}
	return false;
}

bool KRenderCore::UpdateFrameTime()
{
	KRenderGlobal::Statistics.Update();

	KRenderStatistics statistics;
	KRenderGlobal::Statistics.GetAllStatistics(statistics);

	char szBuffer[1024] = {};
	sprintf(szBuffer, "[FPS] %f [FrameTime] %f", statistics.frame.fps, statistics.frame.frametime);
	m_Window->SetWindowTitle(szBuffer);

	return true;
}

bool KRenderCore::UpdateUIOverlay(size_t frameIndex)
{
	IKUIOverlayPtr ui = m_Device->GetUIOverlay();

	ui->StartNewFrame();
	{
		ui->SetWindowPos(10, 10);
		ui->SetWindowSize(0, 0);
		ui->Begin("Example");
		{
			KRenderStatistics statistics;
			KRenderGlobal::Statistics.GetAllStatistics(statistics);

			ui->Text("FPS [%f] FrameTime [%f]", statistics.frame.fps, statistics.frame.frametime);
			ui->Text("DrawCall [%d] Face [%d] [Primtives] [%d]", statistics.stage.drawcalls, statistics.stage.faces, statistics.stage.primtives);
			ui->PushItemWidth(110.0f);
			if (ui->Header("Setting"))
			{
				ui->CheckBox("MouseCtrlCamera", &m_MouseCtrlCamera);
				ui->CheckBox("OctreeDraw", &m_OctreeDebugDraw);
				ui->CheckBox("MultiRender", &m_MultiThreadSumbit);
				ui->SliderFloat("Shadow DepthBiasConstant", &KRenderGlobal::CascadedShadowMap.GetDepthBiasConstant(), 0.0f, 5.0f);
				ui->SliderFloat("Shadow DepthBiasSlope", &KRenderGlobal::CascadedShadowMap.GetDepthBiasSlope(), 0.0f, 5.0f);
				ui->SliderFloat("Shadow ShadowRange", &KRenderGlobal::CascadedShadowMap.GetShadowRange(), 0.1f, 1000.0f);
				ui->SliderFloat("Shadow SplitLambda", &KRenderGlobal::CascadedShadowMap.GetSplitLambda(), 0.001f, 1.0f);
				ui->CheckBox("Shadow FixToScene", &KRenderGlobal::CascadedShadowMap.GetFixToScene());
				ui->CheckBox("Shadow FixTexel", &KRenderGlobal::CascadedShadowMap.GetFixTexel());
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
	m_GizmoContoller.Update(dt);

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

void KRenderCore::OnSwapChainRecreate(uint32_t width, uint32_t height)
{
	KRenderGlobal::PostProcessManager.Resize(width, height);
}