#pragma once
#include "Internal/KVertexDefinition.h"
#include "KVulkanConfig.h"

namespace KVulkanHelper
{
	enum MSAASupportTarget
	{
		MST_COLOR,
		MST_DEPTH,
		MST_BOTH,
	};

	struct VulkanBindingDetail
	{
		VkVertexInputBindingDescription bindingDescription;
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
	};
	typedef std::vector<VulkanBindingDetail> VulkanBindingDetailList;

	struct SubRegionCopyInfo
	{
		uint32_t offset;
		uint32_t width; 
		uint32_t height; 
		uint32_t mipLevel;
		uint32_t layer;
	};
	typedef std::vector<SubRegionCopyInfo> SubRegionCopyInfoList;

	bool FindMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties, uint32_t& idx);

	bool TextureTypeToVkImageType(TextureType textureType, VkImageType& imageType, VkImageViewType& imageViewType);
	bool ElementFormatToVkFormat(ElementFormat elementFormat, VkFormat& vkFormat);

	bool AddressModeToVkSamplerAddressMode(AddressMode addressMode, VkSamplerAddressMode& vkAddressMode);
	bool FilterModeToVkFilter(FilterMode filterMode, VkFilter& vkFilterkMode);

	bool TopologyToVkPrimitiveTopology(PrimitiveTopology topology, VkPrimitiveTopology& vkPrimitiveTopology);
	bool PolygonModeToVkPolygonMode(PolygonMode polygonMode, VkPolygonMode& vkPolygonMode);

	bool CompareFuncToVkCompareOp(CompareFunc func, VkCompareOp& op);

	bool CullModeToVkCullMode(CullMode cullMode, VkCullModeFlagBits& vkCullMode);
	bool FrontFaceToVkFrontFace(FrontFace frontFace, VkFrontFace& vkFrontFace);

	bool BlendFactorToVkBlendFactor(BlendFactor blendFactor, VkBlendFactor& vkBlendFactor);
	bool BlendOperatorToVkBlendOp(BlendOperator blendOperator, VkBlendOp& vkBlendOp);

	bool ShaderTypeFlagToVkShaderStageFlagBits(ShaderTypeFlag shaderTypeFlag, VkShaderStageFlagBits& bit);
	bool ShaderTypesToVkShaderStageFlag(ShaderTypes shaderTypes, VkFlags& flags);

	bool PopulateInputBindingDescription(const VertexInputDetail* pData, size_t uCount, VulkanBindingDetailList& list);

	void CopyVkBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

	void CopyVkBufferToVkImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
	void CopyVkBufferToVkImageByRegion(VkBuffer buffer, VkImage image, uint32_t layers, const SubRegionCopyInfoList& copyInfo);

	void TransitionImageLayout(VkImage image, VkFormat format, uint32_t layers, uint32_t mipLevels, VkImageLayout oldLayout, VkImageLayout newLayout);
	void GenerateMipmaps(VkImage image, VkFormat format, int32_t texWidth, int32_t texHeight, uint32_t layers, uint32_t mipLevels);

	void BeginSingleTimeCommand(VkCommandPool commandPool, VkCommandBuffer& commandBuffer);
	void EndSingleTimeCommand(VkQueue queue, VkCommandPool commandPool, VkCommandBuffer& commandBuffer);

	bool FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features, VkFormat& vkFormat);
	bool HasStencilComponent(VkFormat format);

	bool QueryMSAASupport(MSAASupportTarget target, uint32_t msaaCount, VkSampleCountFlagBits& flag);
}