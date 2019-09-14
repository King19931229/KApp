#pragma once
#include "Internal/KVertexDefinition.h"
#include "vulkan/vulkan.h"

namespace KVulkanHelper
{
	struct VulkanBindingDetail
	{
		VkVertexInputBindingDescription bindingDescription;
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
	};
	typedef std::vector<VulkanBindingDetail> VulkanBindingDetailList;

	bool FindMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties, uint32_t& idx);
	bool ElementFormatToVkFormat(ElementFormat elementFormat, VkFormat& vkFormat);
	bool PopulateInputBindingDescription(const KVertexDefinition::VertexBindingDetail* pData, uint32_t nCount, VulkanBindingDetailList& list);
}