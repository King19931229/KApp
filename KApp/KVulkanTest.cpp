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

	ASSERT_RESULT(InitCodecManager());
	ASSERT_RESULT(InitAssetLoaderManager());

	IKRenderWindowPtr window = CreateRenderWindow(RENDER_WINDOW_GLFW);
	window->Init(60, 60, 1280, 720, true);
	IKRenderDevicePtr device = CreateRenderDevice(RENDER_DEVICE_VULKAN);
	device->Init(window.get());

	IKRenderCorePtr renderCore = CreateRenderCore();
	renderCore->Init(device, window);
	renderCore->Loop();
	renderCore->UnInit();

	window->UnInit();
	device->UnInit();

	ASSERT_RESULT(UnInitCodecManager());
	ASSERT_RESULT(UnInitAssetLoaderManager());

	KLog::Logger->UnInit();
	KFileSystem::Manager->UnInit();
}