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

	bool PopulateInputBindingDescription(const VertexFormat* pData, size_t uCount, VulkanBindingDetailList& list);

	bool FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features, VkFormat& vkFormat);
	bool HasStencilComponent(VkFormat format);
	bool FindBestDepthFormat(bool bStencil, VkFormat& format);

	bool QueryMSAASupport(MSAASupportTarget target, uint32_t msaaCount, VkSampleCountFlagBits& flag);

	bool GetBufferDeviceAddress(VkBuffer buffer, VkDeviceAddress& address);
}