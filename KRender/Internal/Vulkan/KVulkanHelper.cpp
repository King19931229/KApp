#include "KVulkanHelper.h"

namespace KVulkanHelper
{
	bool FindMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties, uint32_t& idx)
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

	bool ElementFormatToVkFormat(ElementFormat elementFormat, VkFormat& vkFormat)
	{
		switch (elementFormat)
		{
		case EF_R8GB8BA8_UNORM:
			vkFormat = VK_FORMAT_R8G8B8A8_UNORM;
			return true;
		case EF_R8G8B8A8_SNORM:
			vkFormat = VK_FORMAT_R8G8B8A8_SNORM;
			return true;
		case EF_R16_FLOAT:
			vkFormat = VK_FORMAT_R16_SFLOAT;
			return true;
		case EF_R16G16_FLOAT:
			vkFormat = VK_FORMAT_R16G16_SFLOAT;
			return true;
		case EF_R16G16B16_FLOAT:
			vkFormat = VK_FORMAT_R16G16B16_SFLOAT;
			return true;
		case EF_R16G16B16A16_FLOAT:
			vkFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
			return true;
		case EF_R32_FLOAT:
			vkFormat = VK_FORMAT_R32_SFLOAT;
			return true;
		case EF_R32G32_FLOAT:
			vkFormat = VK_FORMAT_R32G32_SFLOAT;
			return true;
		case EF_R32G32B32_FLOAT:
			vkFormat = VK_FORMAT_R32G32B32_SFLOAT;
			return true;
		case EF_R32G32B32A32_FLOAT:
			vkFormat = VK_FORMAT_R32G32B32A32_SFLOAT;
			return true;
		case EF_R32_UINT:
			vkFormat = VK_FORMAT_R32_UINT;
			return true;
		default:
			vkFormat = VK_FORMAT_UNDEFINED;
			return false;
		}
	}

	bool PopulateInputBindingDescription(const KVertexDefinition::VertexBindingDetail* pData, uint32_t nCount, VulkanBindingDetailList& detailList)
	{
		using KVertexDefinition::VertexDetail;
		using KVertexDefinition::VertexBindingDetail;
		using KVertexDefinition::VertexSemanticDetail;
		using KVertexDefinition::VertexSemanticDetailList;

		if(pData == nullptr || nCount == 0)
			return false;

		detailList.clear();
		for(uint32_t idx = 0; idx < nCount; ++idx)
		{
			const VertexBindingDetail& detail = pData[idx];

			for(VertexFormat format : detail.formats)
			{
				VulkanBindingDetail bindingDetail;

				VkVertexInputBindingDescription& bindingDescription = bindingDetail.bindingDescription;				
				std::vector<VkVertexInputAttributeDescription>& attributeDescriptions = bindingDetail.attributeDescriptions;

				const VertexDetail& vertexDetail = KVertexDefinition::GetVertexDetail(format);

				// 构造VkVertexInputBindingDescription
				bindingDescription.binding = idx;
				bindingDescription.stride = (uint32_t)vertexDetail.vertexSize;
				bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

				// 构造VkVertexInputAttributeDescription
				const VertexSemanticDetailList& semanticDetailList = vertexDetail.semanticDetails;
				for(const VertexSemanticDetail& semanticDetail : semanticDetailList)
				{
					VkVertexInputAttributeDescription attributeDescription = {};
					VkFormat vkFormat = VK_FORMAT_UNDEFINED;
					ElementFormatToVkFormat(semanticDetail.elementFormat, vkFormat);

					attributeDescription.binding = idx;
					// 语意枚举值即为绑定位置
					attributeDescription.location = semanticDetail.semantic;
					attributeDescription.format = vkFormat;
					attributeDescription.offset = (uint32_t)semanticDetail.offset;

					attributeDescriptions.push_back(attributeDescription);
				}

				detailList.push_back(bindingDetail);
			}
		}

		return true;
	}
}