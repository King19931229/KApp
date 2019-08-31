#define MEMORY_DUMP_DEBUG
#include "KBase/Publish/KLockFreeQueue.h"
#include "KBase/Publish/KLockQueue.h"
#include "KBase/Publish/KThreadPool.h"
#include "KBase/Publish/KTimer.h"
#include "KBase/Publish/KSemaphore.h"
#include "KBase/Publish/KTaskExecutor.h"
#include "KBase/Publish/KObjectPool.h"

#include "Interface/IKLog.h"
#include "Publish/KHashString.h"
#include "Publish/KDump.h"

#include "Interface/IKCodec.h"
#include "Interface/IKMemory.h"
IKLogPtr pLog;

#include "KRender/Interface/IKRenderWindow.h"

#include <algorithm>
int main()
{
	DUMP_MEMORY_LEAK_BEGIN();
	IKRenderWindowPtr window = CreateRenderWindow(RD_VULKAN);
	if(window)
	{
		IKRenderDevicePtr device = CreateRenderDevice(RD_VULKAN);

		window->Init(60, 60, 1024, 768);
		device->Init();

		IKRenderDevice::DeviceExtensions ext;
		device->QueryExtensions(ext);
		std::for_each(ext.begin(), ext.end(), [](IKRenderDevice::ExtensionProperties& prop)
		{
			printf("%s\n", prop.property.c_str());
		});
		window->Loop();
		window->UnInit();
	}
}