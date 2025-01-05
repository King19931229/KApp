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

#include "Internal/KRenderGlobal.h"
#include "Internal/KConstantGlobal.h"
#include "Internal/ECS/KECSGlobal.h"

#include "KRenderImGui.h"

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
	m_bSwapChainResized(false),
	m_Device(nullptr),
	m_MainWindow(nullptr),
	m_DebugConsole(nullptr),
	m_Gizmo(nullptr)
{
}

KRenderCore::~KRenderCore()
{
	ASSERT_RESULT(m_SecordaryWindows.empty());
	assert(!m_DebugConsole);
}

bool KRenderCore::InitPostProcess()
{
	size_t width = 0, height = 0;
	m_MainWindow->GetSize(width, height);

	KRenderGlobal::PostProcessManager.Init(width, height, 1, EF_R16G16B16A16_FLOAT, KRenderGlobal::NumFramesInFlight);
#if 0
	auto startPoint = KRenderGlobal::PostProcessManager.GetStartPointPass();

	auto pass = KRenderGlobal::PostProcessManager.CreatePass();
	pass->CastPass()->SetShader("postprocess/screenquad.vert", "postprocess/postprocess.frag");
	pass->CastPass()->SetScale(1.0f);
	pass->CastPass()->SetFormat(EF_R8G8B8A8_UNORM);

	auto pass2 = KRenderGlobal::PostProcessManager.CreatePass();
	pass2->CastPass()->SetShader("postprocess/screenquad.vert", "postprocess/postprocess2.frag");
	pass2->CastPass()->SetScale(1.0f);
	pass2->CastPass()->SetFormat(EF_R8G8B8A8_UNORM);

	auto pass3 = KRenderGlobal::PostProcessManager.CreatePass();
	pass3->CastPass()->SetShader("postprocess/screenquad.vert", "postprocess/postprocess3.frag");
	pass3->CastPass()->SetScale(1.0f);
	pass3->CastPass()->SetFormat(EF_R8G8B8A8_UNORM);

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

	ASSERT_RESULT(m_Device->CreateCommandPool(KRenderGlobal::CommandPool));
	ASSERT_RESULT(KRenderGlobal::CommandPool->Init(QUEUE_GRAPHICS, 0, CBR_RESET_POOL));

	KRenderGlobal::RenderDevice = m_Device;
	KRenderGlobal::FrameResourceManager.Init();
	KRenderGlobal::MeshManager.Init();
	KRenderGlobal::ShaderManager.Init();
	KRenderGlobal::TextureManager.Init();
	KRenderGlobal::SamplerManager.Init();
	KRenderGlobal::PipelineManager.Init();
	KRenderGlobal::MaterialManager.Init();
	KRenderGlobal::DynamicConstantBufferManager.Init(property->uniformBufferOffsetAlignment, property->uniformBufferMaxRange);
	KRenderGlobal::InstanceBufferManager.Init(sizeof(KVertexDefinition::INSTANCE_DATA_MATRIX4F), 65536);
	KRenderGlobal::VirtualGeometryManager.Init();
	KRenderGlobal::VirtualTextureManager.Init(256, 4, 4);

	KRenderGlobal::QuadDataProvider.Init();

	return true;
}

bool KRenderCore::UnInitGlobalManager()
{
	KRenderGlobal::QuadDataProvider.UnInit();

	KRenderGlobal::VirtualTextureManager.UnInit();
	KRenderGlobal::VirtualGeometryManager.UnInit();
	KRenderGlobal::MeshManager.UnInit();
	KRenderGlobal::MaterialManager.UnInit();
	KRenderGlobal::TextureManager.UnInit();
	KRenderGlobal::SamplerManager.UnInit();
	KRenderGlobal::ShaderManager.UnInit();
	KRenderGlobal::PipelineManager.UnInit();
	KRenderGlobal::FrameResourceManager.UnInit();
	KRenderGlobal::DynamicConstantBufferManager.UnInit();
	KRenderGlobal::InstanceBufferManager.UnInit();

	SAFE_UNINIT(KRenderGlobal::CommandPool);
	KRenderGlobal::RenderDevice = nullptr;

	return true;
}

bool KRenderCore::InitRenderer()
{
	size_t width = 0, height = 0;
	m_MainWindow->GetSize(width, height);

	m_CameraCube = CreateCameraCube();
	m_CameraCube->Init(m_Device, &m_Camera);

	KRendererInitContext initContext;
	initContext.camera = &m_Camera;
	initContext.cameraCube = m_CameraCube;
	initContext.width = (uint32_t)width;
	initContext.height = (uint32_t)height;
	initContext.enableAsyncCompute = false;
	initContext.enableMultithreadRender = true;

	KRenderGlobal::Renderer.Init(initContext);

	return true;
}

bool KRenderCore::UnInitRenderer()
{
	KRenderGlobal::Renderer.UnInit();

	m_CameraCube->UnInit();
	m_CameraCube = nullptr;

	return true;
}

bool KRenderCore::InitUISwapChain()
{
	ASSERT_RESULT(m_MainWindow != nullptr);
	m_MainWindow->CreateUISwapChain();

	for (IKRenderWindow* window : m_SecordaryWindows)
	{
		window->CreateUISwapChain();
	}
	return true;
}

bool KRenderCore::InitController()
{
	IKUIOverlay* ui = m_MainWindow->GetUIOverlay();
	m_CameraMoveController.Init(&m_Camera, m_MainWindow, m_Gizmo);
	m_UIController.Init(ui, m_MainWindow);
	m_GizmoContoller.Init(m_Gizmo, m_CameraCube, &m_Camera, m_MainWindow);

	m_KeyCallback = [this](InputKeyboard key, InputAction action)
	{
		ENQUEUE_RENDER_COMMAND(ReloadFromKeyboardInput)([this, key]()
		{
			if (key == INPUT_KEY_ENTER)
			{
				KRenderGlobal::Renderer.GetRHICommandList().Flush(RHICommandFlush::FlushRHIThreadToDone);
				KRenderGlobal::VirtualGeometryManager.Reload();
			}
			if (key == INPUT_KEY_SPACE)
			{
				KRenderGlobal::Renderer.GetRHICommandList().Flush(RHICommandFlush::FlushRHIThreadToDone);
				KRenderGlobal::VirtualTextureManager.Reload();
				// KRenderGlobal::GPUScene.Reload();
			}
			if (key == INPUT_KEY_R)
			{
				KRenderGlobal::Renderer.GetRHICommandList().Flush(RHICommandFlush::FlushRHIThreadToDone);
				KRenderGlobal::PipelineManager.Reload();
			}
		});
	};

#if defined(_WIN32)
	m_MainWindow->RegisterKeyboardCallback(&m_KeyCallback);
#endif
	return true;
}

bool KRenderCore::UnInitUISwapChain()
{
	ASSERT_RESULT(m_MainWindow != nullptr);
	m_MainWindow->DestroyUISwapChain();

	for (IKRenderWindow* window : m_SecordaryWindows)
	{
		window->DestroyUISwapChain();
	}
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
				// TODO
			}
		});
	}

	KRenderGlobal::Scene.InitRenderResource(&m_Camera);

	return true;
}

bool KRenderCore::UnInitRenderResource()
{
	KRenderGlobal::Scene.UnInitRenderResource();

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

bool KRenderCore::Init(IKRenderDevicePtr& device, IKRenderWindowPtr& window, KRenderCoreWindowInit windowInit)
{
	UnInit();

	ENQUEUE_RENDER_COMMAND(KRenderCore_Init)([this, &device, &window, windowInit]()
	{
		if (!m_bInit)
		{
			KShaderMap::InitializePermuationMap();

			m_Device = device.get();
			m_MainWindow = window.get();
			m_DebugConsole = KNEW KDebugConsole();
			m_DebugConsole->Init();

			m_Camera.SetNear(20.0f);
			m_Camera.SetFar(5000.0f);
			m_Camera.SetCustomLockYAxis(glm::vec3(0, 1, 0));
			m_Camera.SetLockYEnable(true);

			KRenderGlobal::MainWindow = window.get();

			m_DeviceInitCallback = [this]()
			{
				InitGlobalManager();
				InitUISwapChain();
				InitPostProcess();
				InitRenderer();
				InitRenderResource();
				InitGizmo();
				InitController();

				for (KRenderCoreInitCallback* callback : m_InitCallbacks)
				{
					(*callback)();
				}

				DebugCode();
			};

			m_DeviceUnInitCallback = [this]()
			{
				KRenderGlobal::Scene.UnInit();

				UnInitController();
				UnInitGizmo();
				UnInitRenderResource();
				UnInitRenderer();
				UnInitPostProcess();
				UnInitUISwapChain();
				UnInitGlobalManager();
			};

			m_MainWindowRenderCB = [this](IKRenderer* dispatcher, uint32_t chainImageIndex)
			{
				dispatcher->SetSceneCamera(&KRenderGlobal::Scene, &m_Camera);
				dispatcher->SetCameraCubeDisplay(true);
			};

			KECSGlobal::Init();
			KRenderGlobal::Scene.Init("GlobalScene", SCENE_MANGER_TYPE_OCTREE, 100000.0f, glm::vec3(0.0f));
			KRenderGlobal::Renderer.SetCallback(m_MainWindow, &m_MainWindowRenderCB);

			m_Device->RegisterDeviceInitCallback(&m_DeviceInitCallback);
			m_Device->RegisterDeviceUnInitCallback(&m_DeviceUnInitCallback);

			GRenderImGui.Open();

			m_bInit = true;
			m_bTickShouldEnd = false;

			return true;
		}
		return false;
	});

	FLUSH_RENDER_COMMAND();
	windowInit(window);

	return true;
}

bool KRenderCore::UnInit()
{
	ENQUEUE_RENDER_COMMAND(KRenderCore_UnInit)([this]()
	{
		if (m_bInit)
		{
			KRenderGlobal::Renderer.RemoveCallback(m_MainWindow);

			KECSGlobal::UnInit();

			m_DebugConsole->UnInit();
			SAFE_DELETE(m_DebugConsole);

			m_Device->UnRegisterDeviceInitCallback(&m_DeviceInitCallback);
			m_Device->UnRegisterDeviceUnInitCallback(&m_DeviceUnInitCallback);

			m_MainWindow = nullptr;
			m_Device = nullptr;

			GRenderImGui.Exit();

			for (IKRenderWindow* window : m_SecordaryWindows)
			{
				window->UnInit();
			}
			m_SecordaryWindows.clear();

			m_Device = nullptr;

			GRenderImGui.Exit();

			m_bInit = false;
			m_bTickShouldEnd = true;

			m_InitCallbacks.clear();
			m_UICallbacks.clear();

			KRenderGlobal::MainWindow = nullptr;
		}
		return true;
	});
	FLUSH_RENDER_COMMAND();

	return true;
}

bool KRenderCore::IsInit() const
{
	return m_bInit;
}

bool KRenderCore::StartRenderThread()
{
	if (!m_RenderThread)
	{
		m_RenderThread = KRunableThreadPtr(new KRunableThread(IKRunablePtr(new KRenderThread), "RenderThread"));
		m_RenderThread->StartUp();
	}
	return true;
}

bool KRenderCore::EndRenderThread()
{
	if (m_RenderThread)
	{
		m_RenderThread->ShutDown();
		m_RenderThread = nullptr;
	}
	return true;
}

bool KRenderCore::TickShouldEnd()
{
	return m_bTickShouldEnd;
}

void KRenderCore::Update()
{
	UpdateFrameTime();
	UpdateUIOverlay();
	UpdateController();
	UpdateGizmo();

	bool enableCameraControl = !GRenderImGui.WantCaptureInput();
	m_CameraMoveController.SetEnable(enableCameraControl);
}

void KRenderCore::Render()
{
	KRenderGlobal::Renderer.GetRHICommandList().Flush(RHICommandFlush::FlushRHIThread);

	KRenderGlobal::CommandPool->Reset();

	uint32_t frameIndex = 0;

	IKSwapChain* mainSwapChain = m_MainWindow->GetSwapChain();
	IKUIOverlay* uiOverlay = m_MainWindow->GetUIOverlay();

	if (m_MainWindow->IsMinimized())
	{
		return;
	}

	if (m_bSwapChainResized)
	{
		KRenderGlobal::Renderer.OnSwapChainRecreate(mainSwapChain->GetWidth(), mainSwapChain->GetHeight());
		if (uiOverlay)
		{
			uiOverlay->Resize(mainSwapChain->GetWidth(), mainSwapChain->GetHeight());
		}
		m_bSwapChainResized = false;
	}

	mainSwapChain->WaitForInFlightFrame(frameIndex);

	KRenderGlobal::CurrentInFlightFrameIndex = frameIndex;

	uint32_t chainImageIndex = 0;
	mainSwapChain->AcquireNextImage(chainImageIndex);

	Update();

	{
		KRenderGlobal::Renderer.SetSwapChain(mainSwapChain);
		KRenderGlobal::Renderer.SetUIOverlay(uiOverlay);

		KRenderGlobal::Renderer.Update();
		KRenderGlobal::Renderer.Execute(chainImageIndex);

		KRenderGlobal::Renderer.GetRHICommandList().Present(mainSwapChain, [this](IKSwapChain* swapChain, bool needResize)
		{
			m_bSwapChainResized = needResize;
			++KRenderGlobal::CurrentFrameNum;
		});
	}

	std::unordered_map<IKRenderWindow*, IKSwapChainPtr> m_SecordaryWindow;

	if (!m_SecordaryWindow.empty())
	{
		// 等待设备空闲 不再进行FrameInFlight
		KRenderGlobal::Renderer.GetRHICommandList().Flush(RHICommandFlush::FlushRHIThreadToDone);
	}

	for (auto& pair : m_SecordaryWindow)
	{
		IKSwapChain* secordarySwapChain = pair.second.get();
		// 处理次交换链的FrameInFlight 但是FrameIndex依然使用主交换链的结果
		uint32_t secordaryFrameIndex = 0;
		secordarySwapChain->WaitForInFlightFrame(secordaryFrameIndex);
		secordarySwapChain->AcquireNextImage(chainImageIndex);

		{
			KRenderGlobal::Renderer.SetSwapChain(secordarySwapChain);
			KRenderGlobal::Renderer.Update();
			KRenderGlobal::Renderer.Execute(chainImageIndex);

			KRenderGlobal::Renderer.GetRHICommandList().Present(secordarySwapChain, [](IKSwapChain* swapChain, bool needResize) {});
		}
	}
}

bool KRenderCore::Tick()
{
	if (m_bInit && !m_bTickShouldEnd)
	{
		if (m_MainWindow->Tick())
		{
			if (KECS::EntityManager)
			{
				KECS::EntityManager->ViewAllEntity([](IKEntityPtr entity) { entity->PreTick(); });
			}

			for (IKRenderWindow* window : m_SecordaryWindows)
			{
				window->Tick();
			}

			ENQUEUE_RENDER_COMMAND(KRenderCore_Render)([this]()
			{
				KRenderGlobal::MaterialManager.Tick();
				Render();
			});

			if (KECS::EntityManager)
			{
				KECS::EntityManager->ViewAllEntity([](IKEntityPtr entity) { entity->PostTick(); });
			}

			m_FrameSync.Sync();

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
		FLUSH_RENDER_COMMAND();
		KRenderGlobal::Renderer.GetRHICommandList().Flush(RHICommandFlush::FlushRHIThreadToDone);
		return true;
	}
	return false;
}

bool KRenderCore::RegisterSecordaryWindow(IKRenderWindowPtr& window)
{
	if (m_bInit)
	{
		if (std::find(m_SecordaryWindows.begin(), m_SecordaryWindows.end(), window.get()) == m_SecordaryWindows.end())
		{
			window->CreateUISwapChain();
			m_SecordaryWindows.push_back(window.get());
			return true;
		}
	}
	return false;
}

bool KRenderCore::UnRegisterSecordaryWindow(IKRenderWindowPtr& window)
{
	if (m_bInit)
	{
		auto it = std::find(m_SecordaryWindows.begin(), m_SecordaryWindows.end(), window.get());
		if (it != m_SecordaryWindows.end())
		{
			m_SecordaryWindows.erase(it);
			window->DestroyUISwapChain();
			SAFE_UNINIT(window);
			return true;
		}
	}
	return false;
}

bool KRenderCore::RegisterInitCallback(KRenderCoreInitCallback* callback)
{
	if (!m_bInit)
	{
		if (callback)
		{
			m_InitCallbacks.insert(callback);
			return true;
		}
	}
	return false;
}

bool KRenderCore::UnRegisterInitCallback(KRenderCoreInitCallback* callback)
{
	if (callback)
	{
		auto it = m_InitCallbacks.find(callback);
		if (it != m_InitCallbacks.end())
		{
			m_InitCallbacks.erase(it);
			return true;
		}
	}
	return false;
}

bool KRenderCore::UnRegistertAllInitCallback()
{
	m_InitCallbacks.clear();
	return true;
}

bool KRenderCore::RegisterUIRenderCallback(KRenderCoreUIRenderCallback* callback)
{
	if (callback)
	{
		m_UICallbacks.insert(callback);
		return true;
	}
	return false;
}

bool KRenderCore::UnRegisterUIRenderCallback(KRenderCoreUIRenderCallback* callback)
{
	if (callback)
	{
		auto it = m_UICallbacks.find(callback);
		if (it != m_UICallbacks.end())
		{
			m_UICallbacks.erase(it);
			return true;
		}
	}
	return false;
}

bool KRenderCore::UnRegistertAllUIRenderCallback()
{
	m_UICallbacks.clear();
	return true;
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

	auto UpdateWindowFPS = [this, statistics]()
	{
		char szBuffer[1024] = {};
		sprintf(szBuffer, "[FPS] %f [FrameTime] %f", statistics.frame.fps, statistics.frame.frametime);
		m_MainWindow->SetWindowTitle(szBuffer);
	};

	if (IsInGameThread())
	{
		UpdateWindowFPS();
	}
	else if (IsInRenderThread())
	{
		GetTaskGraphManager()->CreateAndDispatch
		(
			IKTaskWorkPtr(new KLambdaTaskWork([UpdateWindowFPS]()
			{
				UpdateWindowFPS();
			})),
			NamedThread::GAME_THREAD, {}
		);
	}

	return true;
}

bool KRenderCore::UpdateUIOverlay()
{
	IKUIOverlay* ui = m_MainWindow->GetUIOverlay();
	ui->StartNewFrame();
	GRenderImGui.Run();
	for (KRenderCoreUIRenderCallback* callback : m_UICallbacks)
	{
		(*callback)();
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

#include "ShaderMap/KShaderMap.h"
#include "KBase/Interface/IKSourceFile.h"

void KRenderCore::DebugCode()
{
	KShaderMap shaderMap;

	KShaderMapInitContext initContext;
	initContext.vsFile = "shading/basepass.vert";
	initContext.fsFile = "shading/basepass.frag";

	IKSourceFilePtr materialSourceFile = GetSourceFile();
	materialSourceFile->SetIOHooker(KRenderGlobal::ShaderManager.GetSourceFileIOHooker());
	materialSourceFile->AddIncludeSource(KRenderGlobal::ShaderManager.GetBindingGenerateCode());
	materialSourceFile->Open("material/diffuse.glsl");

	initContext.includeSources = { {"material_generate_code.h", materialSourceFile->GetFinalSource()} };

	VertexFormat formats[] = { VF_POINT_NORMAL_UV };

	shaderMap.Init(initContext, false);
	shaderMap.GetVSShader(formats, ARRAY_SIZE(formats));
	shaderMap.GetFSShader(formats, ARRAY_SIZE(formats), nullptr);

	shaderMap.UnInit();

	//KVirtualTextureResourceRef virtualTexture;
	//KRenderGlobal::VirtualTextureManager.Acqiure("Textures/StreamingAssets/", 64, virtualTexture);
}