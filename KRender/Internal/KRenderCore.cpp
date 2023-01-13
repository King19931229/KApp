#include "KRenderCore.h"

#include "Interface/IKRenderWindow.h"
#include "Interface/IKRenderDevice.h"

#include "Internal/KRenderGlobal.h"

#include "KBase/Interface/IKAssetLoader.h"
#include "KBase/Interface/IKCodec.h"
#include "KBase/Interface/Entity/IKEntityManager.h"
#include "KBase/Publish/KPlatform.h"
#include "KBase/Publish/KTimer.h"

#include "Internal/Object/KDebugDrawer.h"

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
	m_bTickShouldEnd(false),
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

	KRenderGlobal::PostProcessManager.Init(m_Device, width, height, 2, EF_R16G16B16A16_FLOAT, KRenderGlobal::NumFramesInFlight);
#if 0
	auto startPoint = KRenderGlobal::PostProcessManager.GetStartPointPass();

	auto pass = KRenderGlobal::PostProcessManager.CreatePass();
	pass->CastPass()->SetShader("postprocess/screenquad.vert", "postprocess/postprocess.frag");
	pass->CastPass()->SetScale(1.0f);
	pass->CastPass()->SetFormat(EF_R8GB8BA8_UNORM);

	auto pass2 = KRenderGlobal::PostProcessManager.CreatePass();
	pass2->CastPass()->SetShader("postprocess/screenquad.vert", "postprocess/postprocess2.frag");
	pass2->CastPass()->SetScale(1.0f);
	pass2->CastPass()->SetFormat(EF_R8GB8BA8_UNORM);

	auto pass3 = KRenderGlobal::PostProcessManager.CreatePass();
	pass3->CastPass()->SetShader("postprocess/screenquad.vert", "postprocess/postprocess3.frag");
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
	KRenderDeviceProperties* property = nullptr;
	ASSERT_RESULT(m_Device->QueryProperty(&property));

	KRenderGlobal::RenderDevice = m_Device;
	KRenderGlobal::FrameResourceManager.Init();
	KRenderGlobal::MeshManager.Init(m_Device);
	KRenderGlobal::ShaderManager.Init(m_Device);
	KRenderGlobal::TextureManager.Init(m_Device);
	KRenderGlobal::SamplerManager.Init(m_Device);
	KRenderGlobal::MaterialManager.Init(m_Device);
	KRenderGlobal::DynamicConstantBufferManager.Init(m_Device, property->uniformBufferOffsetAlignment, property->uniformBufferMaxRange);
	KRenderGlobal::InstanceBufferManager.Init(m_Device, sizeof(KVertexDefinition::INSTANCE_DATA_MATRIX4F), 65536);

	KRenderGlobal::QuadDataProvider.Init();

	KDebugDrawSharedData::Init();

	return true;
}

bool KRenderCore::UnInitGlobalManager()
{
	KDebugDrawSharedData::UnInit();

	KRenderGlobal::QuadDataProvider.UnInit();

	KRenderGlobal::MaterialManager.UnInit();
	KRenderGlobal::MeshManager.UnInit();
	KRenderGlobal::TextureManager.UnInit();
	KRenderGlobal::SamplerManager.UnInit();
	KRenderGlobal::ShaderManager.UnInit();
	KRenderGlobal::FrameResourceManager.UnInit();
	KRenderGlobal::DynamicConstantBufferManager.UnInit();
	KRenderGlobal::InstanceBufferManager.UnInit();

	KRenderGlobal::RenderDevice = nullptr;

	return true;
}

bool KRenderCore::InitRenderer()
{
	size_t width = 0, height = 0;
	m_Window->GetSize(width, height);

	m_CameraCube = CreateCameraCube();
	m_CameraCube->Init(m_Device, &m_Camera);

	GetRayTraceMgr()->CreateRayTraceScene(KRenderGlobal::RayTraceScene);
	KRenderGlobal::RayTraceScene->Init(&KRenderGlobal::Scene, &m_Camera);

	KRenderGlobal::Renderer.Init(&m_Camera, m_CameraCube, (uint32_t)width, (uint32_t)height);

	return true;
}

bool KRenderCore::UnInitRenderer()
{
	KRenderGlobal::Renderer.UnInit();

	KRenderGlobal::RayTraceScene->UnInit();
	GetRayTraceMgr()->RemoveRayTraceScene(KRenderGlobal::RayTraceScene);
	KRenderGlobal::RayTraceScene = nullptr;

	m_CameraCube->UnInit();
	m_CameraCube = nullptr;

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
			// TODO 通知回Pipeline
		}
		if (key == INPUT_KEY_R)
		{
			m_Device->Wait();
			KRenderGlobal::RayTraceManager.ReloadShader();
			KRenderGlobal::RTAO.ReloadShader();
			// KRenderGlobal::Voxilzer.ReloadShader();
			KRenderGlobal::ClipmapVoxilzer.ReloadShader();
			KRenderGlobal::Scene.GetTerrain()->Reload();
			KRenderGlobal::HiZBuffer.ReloadShader();
			KRenderGlobal::HiZOcclusion.ReloadShader();
			KRenderGlobal::VolumetricFog.Reload();
			KRenderGlobal::ScreenSpaceReflection.ReloadShader();
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
		KShaderMap::InitializePermuationMap();

		m_Device = device.get();
		m_Window = window.get();
		m_DebugConsole = KNEW KDebugConsole();
		m_DebugConsole->Init();

		m_Camera.SetNear(20.0f);
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
			InitRenderer();
			InitRenderResource();
			InitGizmo();
			InitController();

			for (KRenderCoreInitCallback* callback : m_Callbacks)
			{
				(*callback)();
			}

			DebugCode();
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
			KRenderGlobal::Scene.UnInit();

			UnInitController();
			UnInitGizmo();
			UnInitRenderResource();
			UnInitRenderer();
			UnInitPostProcess();
			UnInitGlobalManager();
		};

		m_MainWindowRenderCB = [this](IKRenderer* dispatcher, uint32_t chainImageIndex)
		{
			dispatcher->SetSceneCamera(&KRenderGlobal::Scene, &m_Camera);
			dispatcher->SetCameraCubeDisplay(true);
		};

		KECSGlobal::Init();
		KRenderGlobal::Scene.Init(SCENE_MANGER_TYPE_OCTREE, 100000.0f, glm::vec3(0.0f));
		KRenderGlobal::Renderer.SetCallback(m_Window, &m_MainWindowRenderCB);

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
		KRenderGlobal::Renderer.RemoveCallback(m_Window);

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
			if (KECS::EntityManager)
			{
				KECS::EntityManager->ViewAllEntity([](IKEntityPtr entity) { entity->PreTick(); });
			}

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

			if (KECS::EntityManager)
			{
				KECS::EntityManager->ViewAllEntity([](IKEntityPtr entity) { entity->PostTick(); });
			}

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

IKRayTraceManager* KRenderCore::GetRayTraceMgr()
{
	return &KRenderGlobal::RayTraceManager;
}

IKRenderScene* KRenderCore::GetRenderScene()
{
	return &KRenderGlobal::Scene;
}

IKRenderer* KRenderCore::GetRenderer()
{
	return &KRenderGlobal::Renderer;
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

bool KRenderCore::UpdateUIOverlay()
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
				if (ui->Header("Shadow"))
				{
					 ui->SliderFloat("Shadow DepthBias Slope[0]", &KRenderGlobal::CascadedShadowMap.GetDepthBiasSlope(0), 0.0f, 5.0f);
					 ui->SliderFloat("Shadow DepthBias Slope[1]", &KRenderGlobal::CascadedShadowMap.GetDepthBiasSlope(1), 0.0f, 5.0f);
					 ui->SliderFloat("Shadow DepthBias Slope[2]", &KRenderGlobal::CascadedShadowMap.GetDepthBiasSlope(2), 0.0f, 5.0f);
					 ui->SliderFloat("Shadow DepthBias Slope[3]", &KRenderGlobal::CascadedShadowMap.GetDepthBiasSlope(3), 0.0f, 5.0f);

					// ui->SliderFloat("Shadow ShadowRange", &KRenderGlobal::CascadedShadowMap.GetShadowRange(), 0.1f, 5000.0f);
					// ui->SliderFloat("Shadow SplitLambda", &KRenderGlobal::CascadedShadowMap.GetSplitLambda(), 0.001f, 1.0f);
					// ui->SliderFloat("Shadow LightSize", &KRenderGlobal::CascadedShadowMap.GetLightSize(), 0.0f, 0.1f);
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
				if (ui->Header("RTAO"))
				{
					ui->CheckBox("RTAOEnable", &KRenderGlobal::RTAO.GetEnable());
					ui->CheckBox("RTAODebugDraw", &KRenderGlobal::RTAO.GetDebugDrawEnable());
					ui->SliderFloat("Length of the ray", &KRenderGlobal::RTAO.GetAoParameters().rtao_radius, 0.0f, 20.0f);
					ui->SliderInt("Number of samples at each iteration", &KRenderGlobal::RTAO.GetAoParameters().rtao_samples, 1, 32);
					ui->SliderFloat("Strenth of darkness", &KRenderGlobal::RTAO.GetAoParameters().rtao_power, 0.0001f, 10.0f);
					ui->SliderInt("Attenuate based on distance", &KRenderGlobal::RTAO.GetAoParameters().rtao_distance_based, 0, 1);
				}
				/*
				if (ui->Header("SVOGI"))
				{
					ui->CheckBox("Octree", &KRenderGlobal::Voxilzer.GetVoxelUseOctree());
					ui->CheckBox("VoxelDraw", &KRenderGlobal::Voxilzer.GetVoxelDrawEnable());
					ui->CheckBox("VoxelDrawWireFrame", &KRenderGlobal::Voxilzer.GetVoxelDrawWireFrame());
					ui->CheckBox("LightDraw", &KRenderGlobal::Voxilzer.GetLightDebugDrawEnable());
					ui->CheckBox("OctreeRayTestDraw", &KRenderGlobal::Voxilzer.GetOctreeRayTestDrawEnable());
				}
				*/
				if (ui->Header("ClipmapGI"))
				{
					ui->CheckBox("VoxelDraw2", &KRenderGlobal::ClipmapVoxilzer.GetVoxelDrawEnable());
					ui->CheckBox("VoxelDrawWireFrame2", &KRenderGlobal::ClipmapVoxilzer.GetVoxelDrawWireFrame());
					ui->CheckBox("LightDraw2", &KRenderGlobal::ClipmapVoxilzer.GetLightDebugDrawEnable());
					ui->SliderFloat("VoxelBias", &KRenderGlobal::ClipmapVoxilzer.GetVoxelDrawBias(), 0, 16);
				}
				if (ui->Header("VolumetricFog"))
				{
					ui->SliderFloat("FogStart", &KRenderGlobal::VolumetricFog.GetStart(), 1.0f, 5000.0f);
					ui->SliderFloat("FogDepth", &KRenderGlobal::VolumetricFog.GetDepth(), 1.0f, 5000.0f);
					ui->SliderFloat("FogAnisotropy", &KRenderGlobal::VolumetricFog.GetAnisotropy(), 0.0f, 1.0f);
					ui->SliderFloat("FogDensity", &KRenderGlobal::VolumetricFog.GetDensity(), 0.0f, 1.0f);
				}
				if (ui->Header("SSR"))
				{
					ui->CheckBox("SSRDebugDraw", &KRenderGlobal::ScreenSpaceReflection.GetDebugDrawEnable());
				}
			}
			ui->PopItemWidth();
		}
		ui->End();
	}
	ui->EndNewFrame();

	ui->Update();
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
	UpdateUIOverlay();
	UpdateController();
	UpdateGizmo();

	KRenderGlobal::EnableDebugRender = m_OctreeDebugDraw;
	m_CameraMoveController.SetEnable(m_MouseCtrlCamera);
}

void KRenderCore::OnPostPresent(uint32_t chainIndex, uint32_t frameIndex)
{
	++KRenderGlobal::CurrentFrameNum;
}

void KRenderCore::OnSwapChainRecreate(uint32_t width, uint32_t height)
{
	KRenderGlobal::Renderer.OnSwapChainRecreate(width, height);
}

#include "ShaderMap/KShaderMap.h"
#include "KBase/Interface/IKSourceFile.h"

void KRenderCore::DebugCode()
{
	KShaderMap shaderMap;

	KShaderMapInitContext initContext;
	initContext.vsFile = "shading/basepass.vert";
	initContext.fsFile = "shading/basepass.frag";

	IKFileSystemPtr system = KFileSystem::Manager->GetFileSystem(FSD_SHADER);
	ASSERT_RESULT(system);
	IKSourceFilePtr materialSourceFile = GetSourceFile();
	materialSourceFile->SetIOHooker(IKSourceFile::IOHookerPtr(KNEW KShaderSourceHooker(system)));
	materialSourceFile->Open("material/diffuse.glsl");

	initContext.IncludeSource = { {"material_generate_code.h", materialSourceFile->GetFinalSource()} };

	VertexFormat formats[] = { VF_POINT_NORMAL_UV };

	shaderMap.Init(initContext, false);
	shaderMap.GetVSShader(formats, ARRAY_SIZE(formats));
	shaderMap.GetFSShader(formats, ARRAY_SIZE(formats), nullptr, false);

	shaderMap.UnInit();
}