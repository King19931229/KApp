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

#include "KRender/Interface/IKRenderWindow.h"
#include "KRender/Interface/IKRenderDevice.h"
#include "KRender/Interface/IKShader.h"
#include "KRender/Interface/IKRenderCore.h"

#include <algorithm>
#include <process.h>

#include "KBase/Publish/KNumerical.h"
#include "KBase/Publish/KThreadPool.h"

#include "KBase/Publish/KHash.h"

#include "KRender/Internal/KDebugConsole.h"
#include "Interface/IKFileSystem.h"

int main()
{
	KLog::Logger->Init("log.txt", true, true, ILM_UNIX);
	
	KFileSystem::Manager->Init();
	KFileSystem::Manager->AddSystem("../Sponza.zip", -1, FST_ZIP);
	KFileSystem::Manager->AddSystem(".", 0, FST_NATIVE);
	KFileSystem::Manager->AddSystem("../", 1, FST_NATIVE);

	/*
	KDebugConsole console;
	KDebugConsole::InputCallBackType callback = [](const char* info)
	{
		KG_LOGE(info);
	};
	console.AddCallback(&callback);
	console.Init();
	*/


	IKRenderCorePtr renderCore = CreateRenderCore();

	renderCore->Init(RD_VULKAN, 1280, 720);
	renderCore->Loop();
	renderCore->UnInit();

	KLog::Logger->UnInit();
	KFileSystem::Manager->UnInit();
}