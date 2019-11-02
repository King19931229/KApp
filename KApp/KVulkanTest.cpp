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
#include "KRender/Interface/IKProgram.h"

#include <algorithm>
#include <process.h>

#include "KBase/Publish/KNumerical.h"
#include "KBase/Publish/KThreadPool.h"

int main()
{
	/*
	KThreadPool<std::function<bool()>> threadPool;
	threadPool.PushWorkerThreads(8);
	std::atomic<int> count = 0;
	for(int i = 0; i < 1000; ++i)
	{
		threadPool.SubmitTask([&]()
		{
			printf("work %d\n", count++);
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
			return true;
		});
	}
	threadPool.WaitAllAsyncTaskDone();
	return 0;
	*/
	/*
	printf("%d\n", KNumerical::Pow2LessEqual(10));
	printf("%d\n", KNumerical::Pow2LessEqual(768));
	printf("%d\n", KNumerical::Pow2LessEqual(512));

	printf("%d\n", KNumerical::Pow2GreaterEqual(10));
	printf("%d\n", KNumerical::Pow2GreaterEqual(768));
	printf("%d\n", KNumerical::Pow2GreaterEqual(512));
	printf("%d\n", KNumerical::Pow2GreaterEqual(-1));
	
	printf("%d\n", KNumerical::IsPow2(1));
	printf("%d\n", KNumerical::IsPow2(768));
	printf("%d\n", KNumerical::IsPow2(4));
	printf("%d\n", KNumerical::IsPow2(1024));
	printf("%d\n", KNumerical::IsPow2(1111));
	*/
	/*
	std::vector<std::string> splitResult;
	KStringUtil::Split("I am ;; a string for test;;; ha,ha!", " ;,!", splitResult);
	*/
	/*KSystem::Wait("D:\\VulkanSDK\\1.1.114.0\\Bin\\glslc.exe", "d:\\KApp\\Shader\\shader.vert -o test.txt");
	std::string vulkanRoot = getenv("VK_SDK_PATH");
	vulkanRoot = getenv("VK_SDK_PATH");
	KFileTool::PathJoin(vulkanRoot, "Bin/spirv-as.exe", vulkanRoot);
	*/

	InitCodecManager();
	InitAssetLoaderManager();

	IKAssetLoaderPtr loader = GetAssetLoader();
	KAssetImportOption option;
	option.components.push_back(AVC_POSITION_3F);
	option.components.push_back(AVC_NORMAL_3F);
	option.components.push_back(AVC_UV_2F);
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
}