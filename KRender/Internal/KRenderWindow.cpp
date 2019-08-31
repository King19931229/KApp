#include "Vulkan/KVulkanRenderWindow.h"

IKRenderWindowPtr CreateRenderWindow(RenderDevice platform)
{
	switch (platform)
	{
	case RD_VULKAN:
		return IKRenderWindowPtr((IKRenderWindow*)new KVulkanRenderWindow());
	case RD_COUNT:
	default:
		return IKRenderWindowPtr(nullptr);
	}
}