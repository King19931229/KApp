#define MEMORY_DUMP_DEBUG
#include "KBase/Publish/KLockFreeQueue.h"
#include "KBase/Publish/KLockQueue.h"
#include "KBase/Publish/KThreadPool.h"
#include "KBase/Publish/KTimer.h"
#include "KBase/Publish/KSemaphore.h"
#include "KBase/Publish/KTaskExecutor.h"
#include "KBase/Publish/KObjectPool.h"
#include "KBase/Publish/KSystem.h"
#include "KBase/Publish/KFileTool.h"
#include "KBase/Publish/KStringUtil.h"

#include "Interface/IKLog.h"
#include "Interface/IKAssetLoader.h"
#include "Publish/KHashString.h"
#include "Publish/KDump.h"

#include "Interface/IKCodec.h"
#include "Interface/IKMemory.h"
IKLogPtr pLog;

#include "KRender/Interface/IKRenderWindow.h"
#include "KRender/Interface/IKRenderDevice.h"
#include "KRender/Interface/IKShader.h"

#include <algorithm>
#include <process.h>

#include "KBase/Publish/KNumerical.h"
#include "KBase/Publish/KThreadPool.h"

#include "KBase/Publish/KHash.h"

#include "KRender/Internal/KDebugConsole.h"
#include "Interface/IKFileSystem.h"

int main()
{
	GLogger = CreateLog();
	GLogger->Init("log.txt", true, true, ILM_UNIX);

	GFileSystemManager = CreateFileSystemManager();
	GFileSystemManager->Init();

	GFileSystemManager->AddSystem(".", 0, FST_NATIVE);
	GFileSystemManager->AddSystem("../", 1, FST_NATIVE);

	/*
	KDebugConsole console;
	KDebugConsole::InputCallBackType callback = [](const char* info)
	{
		KG_LOGE(info);
	};
	console.AddCallback(&callback);
	console.Init();
	*/

	InitCodecManager();
	InitAssetLoaderManager();

	IKAssetLoaderPtr loader = GetAssetLoader();

	KAssetImportOption::ComponentGroup group;
	group.push_back(AVC_POSITION_3F);
	group.push_back(AVC_NORMAL_3F);
	group.push_back(AVC_UV_2F);

	KAssetImportOption option;
	option.components.push_back(std::move(group));

	KAssetImportResult result;

	loader->Import("../Dependency/assimp-3.3.1/test/models/OBJ/spider.obj", option, result);

	IKRenderWindowPtr window = CreateRenderWindow(RD_VULKAN);
	if(window)
	{
		IKRenderDevicePtr device = CreateRenderDevice(RD_VULKAN);

		window->Init(60, 60, 1024, 768, true);
		device->Init(window);
		/*
		IKShaderPtr vtShader = nullptr;
		device->CreateShader(vtShader);

		IKShaderPtr fgShader = nullptr;
		device->CreateShader(fgShader);

		IKProgramPtr program = nullptr;
		device->CreateProgram(program);

		if(vtShader->InitFromFile("shader.vert") && fgShader->InitFromFile("shader.frag"))
		{
			program->AttachShader(ST_VERTEX, vtShader);
			program->AttachShader(ST_FRAGMENT, fgShader);
			program->Init();
		}
		*/
		window->Loop();

		device->UnInit();
		window->UnInit();
	}

	UnInitAssetLoaderManager();
	UnInitCodecManager();

	GFileSystemManager->UnInit();
}