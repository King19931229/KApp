#include "KRenderCore.h"

#include "Interface/IKRenderWindow.h"
#include "Interface/IKRenderDevice.h"

#include "Internal/KRenderGlobal.h"

#include "KBase/Interface/IKAssetLoader.h"
#include "KBase/Interface/IKCodec.h"
#include "KBase/Interface/Entity/IKEntityManager.h"
#include "KBase/Publish/KPlatform.h"
#include "KBase/Publish/KTimer.h"

#include "Internal/KConstantGlobal.h"
#include "Internal/ECS/KECSGlobal.h"

#ifdef _WIN32
#	pragma warning(disable:4996)
#endif

EXPORT_DLL IKRenderCorePtr CreateRenderCore()
{
	return IKRenderCorePtr(KNEW KRenderCore());
}

KRenderCore::KRenderCore()
	: m_bInit(false),
	m_Device(nullptr),
	m_Window(nullptr),
	m_DebugConsole(nullptr),
	m_Gizmo(nullptr),
	m_MultiThreadSubmit(true),
	m_InstanceSubmit(true),
	m_OctreeDebugDraw(false),
	m_MouseCtrlCamera(true)
{
}

KRenderCore::~KRenderCore()
{
	ASSERT_RESULT(m_SecordaryWindow.empty());
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
	pass->CastPass()->SetShader("Shaders/screenquad.vert", "postprocess.frag");
	pass->CastPass()->SetScale(1.0f);
	pass->CastPass()->SetFormat(EF_R8GB8BA8_UNORM);

	auto pass2 = KRenderGlobal::PostProcessManager.CreatePass();
	pass2->CastPass()->SetShader("Shaders/screenquad.vert", "postprocess2.frag");
	pass2->CastPass()->SetScale(1.0f);
	pass2->CastPass()->SetFormat(EF_R8GB8BA8_UNORM);

	auto pass3 = KRenderGlobal::PostProcessManager.CreatePass();
	pass3->CastPass()->SetShader("Shaders/screenquad.vert", "postprocess3.frag");
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

	KRenderDeviceProperties property;
	ASSERT_RESULT(m_Device->QueryProperty(property));

	KRenderGlobal::PipelineManager.Init(m_Device);
	KRenderGlobal::FrameResourceManager.Init(m_Device, frameInFlight);
	KRenderGlobal::MeshManager.Init(m_Device, frameInFlight);
	KRenderGlobal::ShaderManager.Init(m_Device);
	KRenderGlobal::TextureManager.Init(m_Device);
	KRenderGlobal::MaterialManager.Init(m_Device);
	KRenderGlobal::DynamicConstantBufferManager.Init(m_Device, frameInFlight, property.uniformBufferOffsetAlignment, property.uniformBufferMaxRange);
	KRenderGlobal::InstanceBufferManager.Init(m_Device, frameInFlight, sizeof(KVertexDefinition::INSTANCE_DATA_MATRIX4F), 65536);

//#if 1
//	KMaterial material;
//	material.Init("color.vert", "color.frag", false);
//	material.SaveAsFile("../../../Missing.mtl");
//	material.UnInit();
//#endif

	KRenderGlobal::SkyBox.Init(m_Device, frameInFlight, "Textures/uffizi_cube.ktx");
	KRenderGlobal::OcclusionBox.Init(m_Device, frameInFlight);
	KRenderGlobal::ShadowMap.Init(m_Device, frameInFlight, 2048);
	KRenderGlobal::CascadedShadowMap.Init(m_Device, frameInFlight, 3, 2048, 1.0f);

	return true;
}

bool KRenderCore::UnInitGlobalManager()
{
	KRenderGlobal::SkyBox.UnInit();
	KRenderGlobal::OcclusionBox.UnInit();
	KRenderGlobal::ShadowMap.UnInit();
	KRenderGlobal::CascadedShadowMap.UnInit();

	KRenderGlobal::MaterialManager.UnInit();
	KRenderGlobal::MeshManager.UnInit();
	KRenderGlobal::TextureManager.UnInit();
	KRenderGlobal::ShaderManager.UnInit();
	KRenderGlobal::PipelineManager.UnInit();
	KRenderGlobal::FrameResourceManager.UnInit();
	KRenderGlobal::DynamicConstantBufferManager.UnInit();
	KRenderGlobal::InstanceBufferManager.UnInit();

	return true;
}

bool KRenderCore::InitRenderDispatcher()
{
	uint32_t frameInFlight = m_Device->GetNumFramesInFlight();

	m_CameraCube = CreateCameraCube();
	m_CameraCube->Init(m_Device, frameInFlight, &m_Camera);

	KRenderGlobal::RenderDispatcher.Init(m_Device, frameInFlight, m_CameraCube);
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
	IKUIOverlay* ui = m_Device->GetUIOverlay();
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
				component->Init(true);
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
		m_DebugConsole = KNEW KDebugConsole();
		m_DebugConsole->Init();

		m_Camera.SetNear(1.0f);
		m_Camera.SetFar(5000.0f);
		m_Camera.SetCustomLockYAxis(glm::vec3(0, 1, 0));
		m_Camera.SetLockYEnable(true);

		m_PrePresentCallback = [this](uint32_t chainIndex, uint32_t frameIndex)
		{
			OnPrePresent(chainIndex, frameIndex);
		};

		m_PostPresentCallback = [this](uint32_t chainIndex, uint32_t frameIndex)
		{
			OnPostPresent(chainIndex, frameIndex);
		};

		m_SwapChainCallback = [this](uint32_t width, uint32_t height)
		{
			OnSwapChainRecreate(width, height);
		};

		m_InitCallback = [this]()
		{
			KRenderGlobal::TaskExecutor.Init("RenderTaskThread", std::thread::hardware_concurrency());
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
			for (auto& pair : m_SecordaryWindow)
			{
				IKRenderWindow* window = pair.first;
				IKSwapChainPtr& swapChain = pair.second;

				ASSERT_RESULT(m_Device->UnRegisterSecordarySwapChain(swapChain.get()));

				window->UnInit();
				swapChain->UnInit();
			}
			m_SecordaryWindow.clear();

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

		m_MainWindowRenderCB = [this](IKRenderDispatcher* dispatcher, uint32_t chainImageIndex, uint32_t frameIndex)
		{
			dispatcher->SetSceneCamera(&KRenderGlobal::Scene, &m_Camera);
			dispatcher->SetCameraCubeDisplay(true);
		};

		KECSGlobal::Init();
		KRenderGlobal::Scene.Init(SCENE_MANGER_TYPE_OCTREE, 100000.0f, glm::vec3(0.0f));
		KRenderGlobal::RenderDispatcher.SetCallback(m_Window, &m_MainWindowRenderCB);

		m_Device->RegisterPrePresentCallback(&m_PrePresentCallback);
		m_Device->RegisterPostPresentCallback(&m_PostPresentCallback);
		m_Device->RegisterSwapChainRecreateCallback(&m_SwapChainCallback);
		m_Device->RegisterDeviceInitCallback(&m_InitCallback);
		m_Device->RegisterDeviceUnInitCallback(&m_UnitCallback);

		m_bInit = true;
		m_bTickShouldEnd = false;

		return true;
	}
	return false;
}

bool KRenderCore::UnInit()
{
	if (m_bInit)
	{
		KRenderGlobal::RenderDispatcher.RemoveCallback(m_Window);

		KRenderGlobal::Scene.UnInit();
		KECSGlobal::UnInit();

		m_DebugConsole->UnInit();
		SAFE_DELETE(m_DebugConsole);

		m_Device->UnRegisterPrePresentCallback(&m_PrePresentCallback);
		m_Device->UnRegisterPostPresentCallback(&m_PostPresentCallback);
		m_Device->UnRegisterSwapChainRecreateCallback(&m_SwapChainCallback);
		m_Device->UnRegisterDeviceInitCallback(&m_InitCallback);
		m_Device->UnRegisterDeviceUnInitCallback(&m_UnitCallback);

		m_Window = nullptr;
		m_Device = nullptr;

		m_bInit = false;
		m_bTickShouldEnd = true;

		m_Callbacks.clear();
	}
	return true;
}

// 只能Loop到主窗口 废弃该做法
bool KRenderCore::Loop()
{
	if (m_bInit)
	{
		m_Window->Loop();
		return true;
	}
	return false;
}

bool KRenderCore::TickShouldEnd()
{
	return m_bTickShouldEnd;
}

bool KRenderCore::Tick()
{
	if (m_bInit && !m_bTickShouldEnd)
	{
		if (m_Window->Tick())
		{
			for (auto it = m_SecordaryWindow.begin(), itEnd = m_SecordaryWindow.end();
				it != itEnd;)
			{
				IKRenderWindow* window = it->first;
				if (!window->Tick())
				{
					IKSwapChainPtr& swapChain = it->second;
					ASSERT_RESULT(m_Device->UnRegisterSecordarySwapChain(swapChain.get()));
					window->UnInit();
					swapChain->UnInit();
					it = m_SecordaryWindow.erase(it);
				}
				else
				{
					++it;
				}
			}
			m_Device->Present();
			return true;
		}
		else
		{
			m_bTickShouldEnd = true;
			return false;
		}
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

bool KRenderCore::RegisterSecordaryWindow(IKRenderWindowPtr& window)
{
	if (m_bInit)
	{
		if (m_SecordaryWindow.find(window.get()) == m_SecordaryWindow.end())
		{
			IKSwapChainPtr swapChain = nullptr;
			ASSERT_RESULT(m_Device->CreateSwapChain(swapChain));
			swapChain->Init(window.get(), 1);
			ASSERT_RESULT(m_Device->RegisterSecordarySwapChain(swapChain.get()));
			m_SecordaryWindow.insert({ window.get() , std::move(swapChain) });
			return true;
		}
	}
	return false;
}

bool KRenderCore::UnRegisterSecordaryWindow(IKRenderWindowPtr& window)
{
	if (m_bInit)
	{
		auto it = m_SecordaryWindow.find(window.get());
		if (it != m_SecordaryWindow.end())
		{
			IKSwapChainPtr& swapChain = it->second;
			ASSERT_RESULT(m_Device->UnRegisterSecordarySwapChain(swapChain.get()));
			SAFE_UNINIT(swapChain);
			m_SecordaryWindow.erase(it);
			SAFE_UNINIT(window);
			return true;
		}
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

IKRenderDispatcher* KRenderCore::GetRenderDispatcher()
{
	return &KRenderGlobal::RenderDispatcher;
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
	IKUIOverlay* ui = m_Device->GetUIOverlay();

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
				ui->CheckBox("MultiRender", &m_MultiThreadSubmit);
				ui->CheckBox("InstanceRender", &m_InstanceSubmit);
				if (ui->Header("Shaodw"))
				{
					ui->SliderFloat("Shadow DepthBias Slope[0]", &KRenderGlobal::CascadedShadowMap.GetDepthBiasSlope(0), 0.0f, 5.0f);
					ui->SliderFloat("Shadow DepthBias Slope[1]", &KRenderGlobal::CascadedShadowMap.GetDepthBiasSlope(1), 0.0f, 5.0f);
					ui->SliderFloat("Shadow DepthBias Slope[2]", &KRenderGlobal::CascadedShadowMap.GetDepthBiasSlope(2), 0.0f, 5.0f);
					ui->SliderFloat("Shadow DepthBias Slope[3]", &KRenderGlobal::CascadedShadowMap.GetDepthBiasSlope(3), 0.0f, 5.0f);

					ui->SliderFloat("Shadow ShadowRange", &KRenderGlobal::CascadedShadowMap.GetShadowRange(), 0.1f, 5000.0f);
					ui->SliderFloat("Shadow SplitLambda", &KRenderGlobal::CascadedShadowMap.GetSplitLambda(), 0.001f, 1.0f);
					ui->SliderFloat("Shadow LightSize", &KRenderGlobal::CascadedShadowMap.GetLightSize(), 0.0f, 0.1f);
					ui->CheckBox("Shadow FixToScene", &KRenderGlobal::CascadedShadowMap.GetFixToScene());
					ui->CheckBox("Shadow FixTexel", &KRenderGlobal::CascadedShadowMap.GetFixTexel());
					ui->CheckBox("Shadow Minimize Draw", &KRenderGlobal::CascadedShadowMap.GetMinimizeShadowDraw());
				}
				if (ui->Header("Hardware Occlusion"))
				{
					ui->CheckBox("Hardware Occlusion Enable", &KRenderGlobal::OcclusionBox.GetEnable());
					ui->SliderFloat("Hardware Occlusion DepthBiasConstant", &KRenderGlobal::OcclusionBox.GetDepthBiasConstant(), -5.0f, 5.0f);
					ui->SliderFloat("Hardware Occlusion DepthBiasSlope", &KRenderGlobal::OcclusionBox.GetDepthBiasSlope(), -5.0f, 5.0f);
					ui->SliderFloat("Hardware Occlusion Instance Size", &KRenderGlobal::OcclusionBox.GetInstanceGroupSize(), 10.0f, 100000.0f);
				}
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

void KRenderCore::OnPrePresent(uint32_t chainIndex, uint32_t frameIndex)
{
	KRenderGlobal::CurrentFrameIndex = frameIndex;

	KRenderGlobal::TaskExecutor.ProcessSyncTask();

	UpdateFrameTime();
	UpdateUIOverlay(frameIndex);
	UpdateController();
	UpdateGizmo();

	KRenderGlobal::Scene.EnableDebugRender(m_OctreeDebugDraw);
	KRenderGlobal::RenderDispatcher.SetMultiThreadSubmit(m_MultiThreadSubmit);
	KRenderGlobal::RenderDispatcher.SetInstanceSubmit(m_InstanceSubmit);
	m_CameraMoveController.SetEnable(m_MouseCtrlCamera);
}

void KRenderCore::OnPostPresent(uint32_t chainIndex, uint32_t frameIndex)
{
	++KRenderGlobal::CurrentFrameNum;
}

void KRenderCore::OnSwapChainRecreate(uint32_t width, uint32_t height)
{
	KRenderGlobal::PostProcessManager.Resize(width, height);
}