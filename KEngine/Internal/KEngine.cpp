#define MEMORY_DUMP_DEBUG
#include "KEngine.h"
#include "KBase/Interface/IKLog.h"
#include "KBase/Interface/Component/IKComponentManager.h"
#include "KBase/Interface/Entity/IKEntityManager.h"
#include "KBase/Interface/IKFileSystem.h"
#include "KBase/Interface/IKAssetLoader.h"
#include "KBase/Interface/IKCodec.h"
#include "KBase/Publish/KPlatform.h"

namespace KEngineGlobal
{
	void CreateEngine()
	{
		if (!Engine)
		{
			Engine = IKEnginePtr(new KEngine());
		}
	}
	void DestroyEngine()
	{
		if (Engine)
		{
			Engine = nullptr;
		}
	}
	IKEnginePtr Engine = nullptr;
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

		// TODO 参数化控制
#if defined(_WIN32)
		{
			IKFileSystemPtr resourceFileSys = KFileSystem::CreateFileSystem(FST_MULTI);

			IKFileSystemPtr subSystem = nullptr;

			subSystem = KFileSystem::CreateFileSystem(FST_ZIP);
			subSystem->SetRoot("../Sponza.zip");
			resourceFileSys->AddSubFileSystem(subSystem, -1);

			subSystem = KFileSystem::CreateFileSystem(FST_NATIVE);
			subSystem->SetRoot(".");
			resourceFileSys->AddSubFileSystem(subSystem, 0);

			subSystem = KFileSystem::CreateFileSystem(FST_NATIVE);
			subSystem->SetRoot("../");
			resourceFileSys->AddSubFileSystem(subSystem, 1);

			KFileSystem::Manager->SetFileSystem(FSD_RESOURCE, resourceFileSys);
		}
		{
			IKFileSystemPtr shaderFileSys = KFileSystem::CreateFileSystem(FST_NATIVE);
			shaderFileSys->SetRoot(".");
			KFileSystem::Manager->SetFileSystem(FSD_SHADER, shaderFileSys);
		}
#elif defined(__ANDROID__)
		{
			IKFileSystemPtr resourceFileSys = KFileSystem::CreateFileSystem(FST_MULTI);

			IKFileSystemPtr subSystem = nullptr;

			subSystem = KFileSystem::CreateFileSystem(FST_ZIP);
			std::string zipPath = std::string(KPlatform::GetExternalDataPath()) + "/Model/Sponza.zip";
			subSystem->SetRoot(zipPath.c_str());
			resourceFileSys->AddSubFileSystem(subSystem, -1);

			subSystem = KFileSystem::CreateFileSystem(FST_APK);
			subSystem->SetRoot(".");
			resourceFileSys->AddSubFileSystem(subSystem, 0);

			subSystem = KFileSystem::CreateFileSystem(FST_NATIVE);
			subSystem->SetRoot(KPlatform::GetExternalDataPath());
			resourceFileSys->AddSubFileSystem(subSystem, 1);

			KFileSystem::Manager->SetFileSystem(FSD_RESOURCE, resourceFileSys);
		}
		{
			IKFileSystemPtr shaderFileSys = KFileSystem::CreateFileSystem(FST_APK);
			shaderFileSys->SetRoot(".");
			KFileSystem::Manager->SetFileSystem(FSD_SHADER, shaderFileSys);
		}
#endif

		KFileSystem::Manager->Init();

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
		KFileSystem::Manager->UnSetAllFileSystem();
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

bool KEngine::Wait()
{
	if (m_RenderCore)
	{
		m_RenderCore->Wait();
		return true;
	}
	return false;
}