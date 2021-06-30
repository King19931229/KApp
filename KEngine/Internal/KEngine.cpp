#define MEMORY_DUMP_DEBUG
#include "KEngine.h"
#include "KScene.h"
#include "KBase/Interface/IKLog.h"
#include "KBase/Interface/Component/IKComponentManager.h"
#include "KBase/Interface/Entity/IKEntityManager.h"
#include "KBase/Interface/IKFileSystem.h"
#include "KBase/Interface/IKAssetLoader.h"
#include "KBase/Interface/IKCodec.h"
#include "KBase/Interface/IKDataStream.h"
#include "KBase/Interface/IKIniFile.h"
#include "KBase/Publish/KStringUtil.h"
#include "KBase/Publish/KPlatform.h"

namespace KEngineGlobal
{
	void CreateEngine()
	{
		if (!Engine)
		{
			Engine = IKEnginePtr(KNEW KEngine());
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
	ASSERT_RESULT(!m_Scene);
	ASSERT_RESULT(!m_bInit);
}

IKRenderCore* KEngine::GetRenderCore()
{
	return m_RenderCore ? m_RenderCore.get() : nullptr;
}

IKScene* KEngine::GetScene()
{
	return m_Scene ? m_Scene.get() : nullptr;
}

static bool ReadFileSystemPathElement(const std::string& element, FileSystemType& type, std::string& path)
{
	if (!element.empty() && element.substr(0, 1) == "[")
	{
		std::string::size_type pos = element.find_first_of("]", 0);
		if (pos != std::string::npos && pos != element.length() - 1)
		{
			std::string typeString = element.substr(1, pos - 1);
			std::string pathString = element.substr(pos + 1);

			type = KFileSystem::StringToFileSystemType(typeString.c_str());
			path = pathString;
			return true;
		}
	}
	return false;
}

static bool WriteFileSystemPathElement(std::string& element, const FileSystemType& type, const std::string& path)
{
	if (!path.empty() && type != FST_MULTI)
	{
		element = "[" + std::string(KFileSystem::FileSystemTypeToString(type)) + "]" + path;
		return true;
	}
	else
	{
		return false;
	}
}

bool KEngine::Init(IKRenderWindowPtr window, const KEngineOptions& options)
{
	UnInit();

	if (window)
	{
		ASSERT_RESULT(options.window.type != KEngineOptions::WindowInitializeInformation::TYPE_UNKNOWN);

		KLog::CreateLogger();
		KLog::Logger->Init("log.txt", true, true, ILM_UNIX);

		m_RenderDoc.Init();

		KECS::CreateComponentManager();
		KECS::CreateEntityManager();

		KFileSystem::CreateFileManager();

#if defined(_WIN32)
		{
			IKIniFilePtr configIni = GetIniFile();
			if (configIni->Open("../../../KConfig.ini"))
			{
				char szBuffer[1024] = { 0 };
				// Resources [FST_MULTI]
				{
					IKFileSystemPtr resourceFileSys = KFileSystem::CreateFileSystem(FST_MULTI);
					if (configIni->GetString("FileSystem", "Resources", szBuffer, sizeof(szBuffer) - 1))
					{
						std::string data = szBuffer;
						std::vector<std::string> pathElements;
						if (KStringUtil::Split(data, ";", pathElements))
						{
							for (const std::string& pathElement : pathElements)
							{
								int priority = 0;
								FileSystemType type = FST_NATIVE;
								std::string path;

								if (ReadFileSystemPathElement(pathElement, type, path))
								{
									IKFileSystemPtr subSystem = nullptr;
									subSystem = KFileSystem::CreateFileSystem(type);
									subSystem->SetRoot(path);
									resourceFileSys->AddSubFileSystem(subSystem, priority++);
								}
								else
								{
									assert(false && "configure file broken");
								}
							}
						}
					}
					KFileSystem::Manager->SetFileSystem(FSD_RESOURCE, resourceFileSys);
				}
				// Shader [FST_NATIVE]
				{
					IKFileSystemPtr shaderFileSys = KFileSystem::CreateFileSystem(FST_NATIVE);
					if (configIni->GetString("FileSystem", "Shader", szBuffer, sizeof(szBuffer) - 1))
					{
						std::string path = szBuffer;
						shaderFileSys->SetRoot(path);
					}
					else
					{
						assert(false && "configure file broken");
					}
					KFileSystem::Manager->SetFileSystem(FSD_SHADER, shaderFileSys);
				}
				// Backup [FST_NATIVE]
				{
					IKFileSystemPtr backupSys = KFileSystem::CreateFileSystem(FST_NATIVE);
					if (configIni->GetString("FileSystem", "Backup", szBuffer, sizeof(szBuffer) - 1))
					{
						std::string path = szBuffer;
						backupSys->SetRoot(path);
					}
					else
					{
						assert(false && "configure file broken");
					}
					KFileSystem::Manager->SetFileSystem(FSD_BACKUP, backupSys);
				}
			}
			else
			{
				assert(false && "configure file not found");
			}
		}
#elif defined(__ANDROID__) //TODO
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
		{
			IKFileSystemPtr backupFileSys = KFileSystem::CreateFileSystem(FST_APK);
			backupFileSys->SetRoot(".");
			KFileSystem::Manager->SetFileSystem(FSD_BACKUP, backupFileSys);
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
			ASSERT_RESULT(m_Window->Init(windowInfo.top, windowInfo.left, windowInfo.width, windowInfo.height, windowInfo.resizable, true));
			break;
		case KEngineOptions::WindowInitializeInformation::TYPE_ANDROID:
			ASSERT_RESULT(m_Window->Init(windowInfo.app));
			break;
		case KEngineOptions::WindowInitializeInformation::TYPE_EDITOR:
			ASSERT_RESULT(m_Window->Init(windowInfo.hwnd, true));
			break;
		default:
			assert(false && "should not reach");
			break;
		}

		m_Scene = IKScenePtr(KNEW KScene());
		m_Scene->Init(m_RenderCore->GetRenderScene());

		m_bInit = true;

		return true;
	}
	return false;
}

bool KEngine::UnInit()
{
	if (m_bInit)
	{
		m_Device->Wait();

		KECS::DestroyEntityManager();
		KECS::DestroyComponentManager();

		m_Window->UnInit();

		m_Scene->UnInit();
		m_Scene = nullptr;

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

		m_RenderDoc.UnInit();

		m_bInit = false;

		return true;
	}
	return false;
}

bool KEngine::Loop()
{
	while (!m_RenderCore->TickShouldEnd())
	{
		m_RenderCore->Tick();
	}
	return true;
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