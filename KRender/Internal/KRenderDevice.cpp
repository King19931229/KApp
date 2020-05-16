#include "Interface/IKRenderDevice.h"
#include "Vulkan/KVulkanRenderDevice.h"

IKRenderDevicePtr CreateRenderDevice(RenderDevice platform)
{
	switch (platform)
	{
	case RENDER_DEVICE_VULKAN:
		return IKRenderDevicePtr((IKRenderDevice*) KNEW KVulkanRenderDevice());
	default:
		assert(false && "impossible to reach");
		return IKRenderDevicePtr(nullptr);
		break;
	}
}