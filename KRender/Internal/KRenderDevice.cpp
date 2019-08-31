#include "Interface/IKRenderDevice.h"
#include "Vulkan/KVulkanRenderDevice.h"

IKRenderDevicePtr CreateRenderDevice(RenderDevice platform)
{
	switch (platform)
	{
	case RD_VULKAN:
		return IKRenderDevicePtr((IKRenderDevice*)new KVulkanRenderDevice());
	case RD_COUNT:
	default:
		return IKRenderDevicePtr(nullptr);
		break;
	}
}