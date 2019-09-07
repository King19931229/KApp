#define MEMORY_DUMP_DEBUG
#include "KBase/Publish/KLockFreeQueue.h"
#include "KBase/Publish/KLockQueue.h"
#include "KBase/Publish/KThreadPool.h"
#include "KBase/Publish/KTimer.h"
#include "KBase/Publish/KSemaphore.h"
#include "KBase/Publish/KTaskExecutor.h"
#include "KBase/Publish/KObjectPool.h"
#include "KBase/Publish/KProcess.h"
#include "KBase/Publish/KFileTool.h"

#include "Interface/IKLog.h"
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
int main()
{
	DUMP_MEMORY_LEAK_BEGIN(); 

	/*KProcess::Wait("D:\\VulkanSDK\\1.1.114.0\\Bin\\glslc.exe", "d:\\KApp\\Shader\\shader.vert -o test.txt");
	std::string vulkanRoot = getenv("VK_SDK_PATH");
	vulkanRoot = getenv("VK_SDK_PATH");
	KFileTool::PathJoin(vulkanRoot, "Bin/spirv-as.exe", vulkanRoot);
	*/
	IKRenderWindowPtr window = CreateRenderWindow(RD_VULKAN);
	if(window)
	{
		IKRenderDevicePtr device = CreateRenderDevice(RD_VULKAN);

		window->Init(60, 60, 1024, 768);
		device->Init(window);

		IKShaderPtr vtShader = nullptr;
		device->CreateShader(vtShader);

		IKShaderPtr fgShader = nullptr;
		device->CreateShader(fgShader);

		IKProgramPtr program = nullptr;
		device->CreateProgram(program);

		if(vtShader->InitFromFile("D:/KApp/Shader/shader.vert") && fgShader->InitFromFile("D:/KApp/Shader/shader.frag"))
		{
			program->AttachShader(ST_VERTEX, vtShader);
			program->AttachShader(ST_FRAGMENT, fgShader);
			program->Init();
		}

		window->Loop();

		device->UnInit();
		window->UnInit();
	}
}