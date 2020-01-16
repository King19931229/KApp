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
#include "KRender/Interface/IKRenderCore.h"

#include <algorithm>

#include "KBase/Publish/KNumerical.h"
#include "KBase/Publish/KThreadPool.h"

#include "KBase/Publish/KHash.h"
#include "KBase/Publish/KPlatform.h"

#include "KRender/Internal/KDebugConsole.h"
#include "Interface/IKFileSystem.h"

void android_main(android_app *state)
{
	DUMP_MEMORY_LEAK_BEGIN();

	KPlatform::AndroidApp = state;

	KLog::Logger->Init("log.txt", true, true, ILM_UNIX);

	KFileSystem::Manager->AddSystem(KPlatform::GetExternalDataPath(), -1, FST_NATIVE);
	std::string zipPath = std::string(KPlatform::GetExternalDataPath())  + "/Model/Sponza.zip";
	KFileSystem::Manager->AddSystem(zipPath.c_str(), -3, FST_ZIP);
	KFileSystem::Manager->AddSystem(".", -2, FST_APK);

	IKRenderWindowPtr window = CreateRenderWindow(RENDER_WINDOW_ANDROID_NATIVE);
	IKRenderDevicePtr device = CreateRenderDevice(RENDER_DEVICE_VULKAN);

	ASSERT_RESULT(InitCodecManager());
	ASSERT_RESULT(InitAssetLoaderManager());

	window->Init(state);
	window->SetRenderDevice(device.get());

	IKRenderCorePtr renderCore = CreateRenderCore();

	renderCore->Init(device, window);

	renderCore->Loop();
	renderCore->UnInit();

	ASSERT_RESULT(UnInitCodecManager());
	ASSERT_RESULT(UnInitAssetLoaderManager());

	KLog::Logger->UnInit();
	KFileSystem::Manager->UnInit();
}