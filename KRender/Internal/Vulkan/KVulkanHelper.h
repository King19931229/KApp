#pragma once
#include "Internal/KVertexDefinition.h"
#include "KVulkanConfig.h"

namespace KVulkanHelper
{
	struct VulkanBindingDetail
	{
		VkVertexInputBindingDescription bindingDescription;
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
	};
	typedef std::vector<VulkanBindingDetail> VulkanBindingDetailList;

	bool FindMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties, uint32_t& idx);

	bool TextureTypeToVkImageType(TextureType textureType, VkImageType& imageType, VkImageViewType& imageViewType);
	bool ElementFormatToVkFormat(ElementFormat elementFormat, VkFormat& vkFormat);
	bool AddressModeToVkSamplerAddressMode(AddressMode addressMode, VkSamplerAddressMode& vkAddressMode);
	bool FilterModeToVkFilter(FilterMode filterMode, VkFilter& vkFilterkMode);

	bool PopulateInputBindingDescription(const KVertexDefinition::VertexBindingDetail* pData, uint32_t nCount, VulkanBindingDetailList& list);

	void CopyVkBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
	void CopyVkBufferToVkImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
	void TransitionImageLayout(VkImage image, VkFormat format, uint32_t mipLevels, VkImageLayout oldLayout, VkImageLayout newLayout);
	void GenerateMipmaps(VkImage image, VkFormat format, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);

	void BeginSingleTimeCommand(VkCommandPool commandPool, VkCommandBuffer& commandBuffer);
	void EndSingleTimeCommand(VkQueue queue, VkCommandPool commandPool, VkCommandBuffer& commandBuffer);

	bool FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features, VkFormat& vkFormat);
	bool HasStencilComponent(VkFormat format);
}