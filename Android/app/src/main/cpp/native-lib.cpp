#include <jni.h>
#include <string>

extern "C" JNIEXPORT jstring JNICALL
Java_com_king_kapp_MainActivity_stringFromJNI(
		JNIEnv *env,
		jobject /* this */) {
	std::string hello = "Hello from C++";
	return env->NewStringUTF(hello.c_str());
}

#include <android/sensor.h>
#include <android/log.h>
#include <android_native_app_glue.h>

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

#include <algorithm>

#include "KBase/Publish/KNumerical.h"
#include "KBase/Publish/KThreadPool.h"

#include "KBase/Publish/KHash.h"
#include "KBase/Publish/KPlatform.h"

#include "KRender/Internal/KDebugConsole.h"
#include "KRender/Internal/Vulkan/KVulkanRenderWindow.h"
#include "Interface/IKFileSystem.h"

void android_main(android_app *state)
{
	DUMP_MEMORY_LEAK_BEGIN();

	KPlatform::AndroidApp = state;

	KLog::Logger->Init("log.txt", true, true, ILM_UNIX);

	KFileSystem::Manager->AddSystem(KPlatform::GetExternalDataPath(), -1, FST_NATIVE);
	KFileSystem::Manager->AddSystem(".", -2, FST_APK);

	InitCodecManager();
	InitAssetLoaderManager();

	IKRenderWindowPtr window = CreateRenderWindow(RD_VULKAN);
	if (window)
	{
		window->Init(state);
		KVulkanRenderWindow *vulkanWindow = (KVulkanRenderWindow *) window.get();
		state->onAppCmd = vulkanWindow->HandleAppCommand;
		state->onInputEvent = vulkanWindow->HandleAppInput;
		state->userData = vulkanWindow;

		IKRenderDevicePtr device = CreateRenderDevice(RD_VULKAN);

		vulkanWindow->Init(state);
		vulkanWindow->SetVulkanDevice((KVulkanRenderDevice *) device.get());
		//device->Init(window);

		window->Loop();

		//device->UnInit();
		window->UnInit();
	}

	UnInitAssetLoaderManager();
	UnInitCodecManager();

	KLog::Logger->UnInit();
}