#pragma once
#include "vulkan/vulkan.h"
#include "GLFW/glfw3.h"

namespace KVulkanHelper
{
	static bool FindMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties, uint32_t& idx)
	{
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
		{
			// 1.typeFilter是否与内存类型兼容
			// 2.propertyFlags内存属性是否完整包含所需properties
			if ((typeFilter & (1 << i))	&& (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
			{
				idx = i;
				return true;
			}
		}
		return false;
	}
}