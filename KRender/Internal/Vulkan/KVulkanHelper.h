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

	bool FindMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties, uint32_t& idx);

	bool TextureTypeToVkImageType(TextureType textureType, VkImageType& imageType, VkImageViewType& imageViewType);
	bool ElementFormatToVkFormat(ElementFormat elementFormat, VkFormat& vkFormat);

	bool AddressModeToVkSamplerAddressMode(AddressMode addressMode, VkSamplerAddressMode& vkAddressMode);
	bool FilterModeToVkFilter(FilterMode filterMode, VkFilter& vkFilterkMode);

	bool TopologyToVkPrimitiveTopology(PrimitiveTopology topology, VkPrimitiveTopology& vkPrimitiveTopology);
	bool PolygonModeToVkPolygonMode(PolygonMode polygonMode, VkPolygonMode& vkPolygonMode);

	bool CompareFuncToVkCompareOp(CompareFunc func, VkCompareOp& op);
	bool StencilOperatorToVkStencilOp(StencilOperator stencilOperator, VkStencilOp& vkStencilOp);

	bool CullModeToVkCullMode(CullMode cullMode, VkCullModeFlagBits& vkCullMode);
	bool FrontFaceToVkFrontFace(FrontFace frontFace, VkFrontFace& vkFrontFace);

	bool BlendFactorToVkBlendFactor(BlendFactor blendFactor, VkBlendFactor& vkBlendFactor);
	bool BlendOperatorToVkBlendOp(BlendOperator blendOperator, VkBlendOp& vkBlendOp);

	bool LoadOpToVkAttachmentLoadOp(LoadOperation op, VkAttachmentLoadOp& vkLoadOp);
	bool StoreOpToVkAttachmentStoreOp(StoreOperation op, VkAttachmentStoreOp& vkStoreOp);

	bool QueryTypeToVkQueryType(QueryType queryType, VkQueryType& vkQueryType);

	bool ShaderTypeFlagToVkShaderStageFlagBits(ShaderType shaderTypeFlag, VkShaderStageFlagBits& bit);
	bool ShaderTypesToVkShaderStageFlag(ShaderTypes shaderTypes, VkFlags& flags);

	bool ImageLayoutToVkImageLayout(ImageLayout layout, VkImageLayout& vkImageLayout);

	bool PopulateInputBindingDescription(const VertexFormat* pData, size_t uCount, VulkanBindingDetailList& list);

	bool FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features, VkFormat& vkFormat);
	bool HasDepthComponent(VkFormat format);
	bool HasStencilComponent(VkFormat format);
	bool FindBestDepthFormat(bool bStencil, VkFormat& format);

	bool QueryMSAASupport(MSAASupportTarget target, uint32_t msaaCount, VkSampleCountFlagBits& flag);

	bool GetBufferDeviceAddress(VkBuffer buffer, VkDeviceAddress& address);

#ifdef VK_USE_DEBUG_UTILS_AS_DEBUG_MARKER
	void DebugUtilsSetObjectName(VkDevice device, uint64_t object, VkObjectType objectType, const char* name);
	void DebugUtilsSetObjectTag(VkDevice device, uint64_t object, VkObjectType objectType, uint64_t name, size_t tagSize, const void* tag);
	void DebugUtilsBeginRegion(VkCommandBuffer cmdbuffer, const char* pMarkerName, const glm::vec4& color);
	void DebugUtilsInsert(VkCommandBuffer cmdbuffer, const std::string& markerName, const glm::vec4& color);
	void DebugUtilsEndRegion(VkCommandBuffer cmdBuffer);
#else
	// https://github.com/SaschaWillems/Vulkan/tree/master/examples/debugmarker
	void DebugMarkerSetObjectName(VkDevice device, uint64_t object, VkDebugReportObjectTypeEXT objectType, const char* name);
	void DebugMarkerSetObjectTag(VkDevice device, uint64_t object, VkDebugReportObjectTypeEXT objectType, uint64_t name, size_t tagSize, const void* tag);
	void DebugMarkerBeginRegion(VkCommandBuffer cmdbuffer, const char* pMarkerName, const glm::vec4& color);
	void DebugMarkerInsert(VkCommandBuffer cmdbuffer, const std::string& markerName, const glm::vec4& color);
	void DebugMarkerEndRegion(VkCommandBuffer cmdBuffer);
#endif
}