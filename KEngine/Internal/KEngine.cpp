#define MEMORY_DUMP_DEBUG
#include "KEngine.h"
#include "KBase/Interface/IKLog.h"
#include "KBase/Interface/Component/IKComponentManager.h"
#include "KBase/Interface/Entity/IKEntityManager.h"
#include "KBase/Interface/IKFileSystem.h"
#include "KBase/Interface/IKAssetLoader.h"
#include "KBase/Interface/IKCodec.h"
#include "KBase/Publish/KPlatform.h"

IKEnginePtr CreateEngine()
{
	return IKEnginePtr(new KEngine());
}

KEngine::KEngine()
	: m_bInit(false)
{
}

KEngine::~KEngine()
{
	ASSERT_RESULT(!m_Device);
	ASSERT_RESULT(!m_Window);
	ASSERT_RESULT(!m_RenderCore);
	ASSERT_RESULT(!m_bInit);
}

IKRenderCore* KEngine::GetRenderCore()
{
	return m_RenderCore ? m_RenderCore.get() : nullptr;
}

bool KEngine::Init(IKRenderWindowPtr window, const KEngineOptions& options)
{
	UnInit();

	if (window)
	{
		ASSERT_RESULT(options.window.type != KEngineOptions::WindowInitializeInformation::TYPE_UNKNOWN);

		KLog::CreateLogger();
		KLog::Logger->Init("log.txt", true, true, ILM_UNIX);

		KECS::CreateComponentManager();
		KECS::CreateEntityManager();

		KFileSystem::CreateFileManager();
		KFileSystem::Manager->Init();

		// TODO 参数化控制
#if defined(_WIN32)
		KFileSystem::Manager->AddSystem("../Sponza.zip", -1, FST_ZIP);
		KFileSystem::Manager->AddSystem(".", 0, FST_NATIVE);
		KFileSystem::Manager->AddSystem("../", 1, FST_NATIVE);
#elif defined(__ANDROID__)
		std::string zipPath = std::string(KPlatform::GetExternalDataPath()) + "/Model/Sponza.zip";
		KFileSystem::Manager->AddSystem(zipPath.c_str(), -1, FST_ZIP);
		KFileSystem::Manager->AddSystem(".", 0, FST_APK);
		KFileSystem::Manager->AddSystem(KPlatform::GetExternalDataPath(), 1, FST_NATIVE);
#endif

		KAssetLoaderManager::CreateAssetLoader();
		KCodec::CreateCodecManager();

		m_Window = std::move(window);
		m_Device = CreateRenderDevice(RENDER_DEVICE_VULKAN);

		m_Window->SetRenderDevice(m_Device.get());

		m_RenderCore = CreateRenderCore();
		m_RenderCore->Init(m_Device, m_Window);

		const auto& windowInfo = options.window;
		switch (windowInfo.type)
		{
		case KEngineOptions::WindowInitializeInformation::TYPE_DEFAULT:
			m_Window->Init(windowInfo.top, windowInfo.left, windowInfo.width, windowInfo.height, windowInfo.resizable);
			break;
		case KEngineOptions::WindowInitializeInformation::TYPE_ANDROID:
			m_Window->Init(windowInfo.app);
			break;
		case KEngineOptions::WindowInitializeInformation::TYPE_EDITOR:
			m_Window->Init(windowInfo.hwnd);
			break;
		default:
			assert(false && "should not reach");
			break;
		}

		m_bInit = true;

		return true;
	}
	return false;
}

bool KEngine::UnInit()
{
	if (m_bInit)
	{
		KECS::DestroyEntityManager();
		KECS::DestroyComponentManager();

		m_Window->UnInit();

		m_RenderCore->UnInit();
		m_RenderCore = nullptr;

		m_Window = nullptr;
		m_Device = nullptr;

		KAssetLoaderManager::DestroyAssetLoader();
		KCodec::DestroyCodecManager();

		KLog::Logger->UnInit();
		KLog::DestroyLogger();

		KFileSystem::Manager->UnInit();
		KFileSystem::DestroyFileManager();

		m_bInit = false;

		return true;
	}
	return false;
}

bool KEngine::Loop()
{
	if (m_RenderCore)
	{
		m_RenderCore->Loop();
		return true;
	}
	return false;
}

bool KEngine::Tick()
{
	if (m_RenderCore)
	{
		m_RenderCore->Tick();
		return true;
	}
	return false;
}