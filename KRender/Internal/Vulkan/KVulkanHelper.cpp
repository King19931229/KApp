#include "KVulkanHelper.h"
#include "KVulkanGlobal.h"
#include "KVulkanInitializer.h"

namespace KVulkanHelper
{
	bool FindMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties, uint32_t& idx)
	{
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
		{
			// 1.typeFilter�Ƿ����ڴ����ͼ���
			// 2.propertyFlags�ڴ������Ƿ�������������properties
			if ((typeFilter & (1 << i))	&& (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
			{
				idx = i;
				return true;
			}
		}
		return false;
	}

	bool TextureTypeToVkImageType(TextureType textureType, VkImageType& imageType, VkImageViewType& imageViewType)
	{
		switch (textureType)
		{
		case TT_TEXTURE_2D:
			imageType = VK_IMAGE_TYPE_2D;
			imageViewType = VK_IMAGE_VIEW_TYPE_2D;
			return true;
		case TT_COUNT:
		default:
			imageType = VK_IMAGE_TYPE_MAX_ENUM;
			imageViewType = VK_IMAGE_VIEW_TYPE_MAX_ENUM;
			assert(false && "Unknown texture type");
			return false;
		}
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
		case EF_R8GB8B8_UNORM:
			vkFormat = VK_FORMAT_R8G8B8_UNORM;
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
			assert(false && "Unknown format");
			return false;
		}
	}

	bool AddressModeToVkSamplerAddressMode(AddressMode addressMode, VkSamplerAddressMode& vkAddressMode)
	{
		switch (addressMode)
		{
		case AM_REPEAT:
			vkAddressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			return true;
		case AM_CLAMP_TO_BORDER:
			vkAddressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
			return true;
		case AM_UNKNOWN:
		default:
			vkAddressMode = VK_SAMPLER_ADDRESS_MODE_MAX_ENUM;
			assert(false && "address mode not supported");
			return false;
		}
	}

	bool TopologyToVkPrimitiveTopology(PrimitiveTopology topology, VkPrimitiveTopology& vkPrimitiveTopology)
	{
		switch (topology)
		{
		case PT_TRIANGLE_LIST:
			vkPrimitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
			return true;
		case PT_TRIANGLE_STRIP:
			vkPrimitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
			return true;
		default:
			assert(false && "topology mode not supported");
			return false;
		}
	}

	bool PolygonModeToVkPolygonMode(PolygonMode polygonMode, VkPolygonMode& vkPolygonMode)
	{
		switch (polygonMode)
		{
		case PM_FILL:
			vkPolygonMode = VK_POLYGON_MODE_FILL;
			return true;
		case PM_LINE:
			vkPolygonMode = VK_POLYGON_MODE_LINE;
			return true;
		case PM_POINT:
			vkPolygonMode = VK_POLYGON_MODE_POINT;
			return true;
		default:
			assert(false && "polygon mode not supported");
			return false;
		}
	}

	bool FilterModeToVkFilter(FilterMode filterMode, VkFilter& vkFilterkMode)
	{
		switch (filterMode)
		{
		case FM_NEAREST:
			vkFilterkMode = VK_FILTER_NEAREST;
			return true;
		case FM_LINEAR:
			vkFilterkMode = VK_FILTER_LINEAR;
			return true;
		case FM_UNKNOWN:
		default:
			assert(false && "address mode not supported");
			return false;
		}
	}

	bool CullModeToVkCullMode(CullMode cullMode, VkCullModeFlagBits& vkCullMode)
	{
		switch (cullMode)
		{
		case CM_NONE:
			vkCullMode = VK_CULL_MODE_NONE;
			return true;
		case CM_FRONT:
			vkCullMode = VK_CULL_MODE_FRONT_BIT;
			return true;
		case CM_BACK:
			vkCullMode = VK_CULL_MODE_BACK_BIT;
			return true;
		default:
			assert(false && "cull mode not supported");
			return false;
		}
	}

	bool FrontFaceToVkFrontFace(FrontFace frontFace, VkFrontFace& vkFrontFace)
	{
		switch (frontFace)
		{
		case FF_COUNTER_CLOCKWISE:
			vkFrontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
			return true;
		case FF_CLOCKWISE:
			vkFrontFace = VK_FRONT_FACE_CLOCKWISE;
			return true;
		default:
			assert(false && "front face not supported");
			return false;
		}
	}

	bool BlendFactorToVkBlendFactor(BlendFactor blendFactor, VkBlendFactor& vkBlendFactor)
	{
		switch (blendFactor)
		{
		case BF_ZEOR:
			vkBlendFactor = VK_BLEND_FACTOR_ZERO;
			return true;
		case BF_ONE:
			vkBlendFactor = VK_BLEND_FACTOR_ONE;
			return true;
		case BF_SRC_COLOR:
			vkBlendFactor = VK_BLEND_FACTOR_SRC_COLOR;
			return true;
		case BF_ONE_MINUS_SRC_COLOR:
			vkBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
			return true;
		default:
			assert(false && "blend factor not supported");
			return false;
		}
	}

	bool BlendOperatorToVkBlendOp(BlendOperator blendOperator, VkBlendOp& vkBlendOp)
	{
		switch (blendOperator)
		{
		case BO_ADD:
			vkBlendOp = VK_BLEND_OP_ADD;
			return true;
		case BO_SUBTRACT:
			vkBlendOp = VK_BLEND_OP_SUBTRACT;
			return true;
		default:
			assert(false && "blend operator not supported");
			return false;
		}
	}

	bool ShaderTypeFlagToVkShaderStageFlagBits(ShaderTypeFlag shaderTypeFlag, VkShaderStageFlagBits& bit)
	{
		switch (shaderTypeFlag)
		{
		case ST_VERTEX:
			bit = VK_SHADER_STAGE_VERTEX_BIT;
			return true;
		case ST_FRAGMENT:
			bit = VK_SHADER_STAGE_FRAGMENT_BIT;
			return true;
		default:
			assert(false && "Unknown shader type flag");
			return false;
			break;
		}
	}

	bool ShaderTypesToVkShaderStageFlag(ShaderTypes shaderTypes, VkFlags& flags)
	{
		flags = 0;
		const ShaderTypeFlag candidate[]  = {ST_VERTEX, ST_FRAGMENT};
		for(ShaderTypeFlag c : candidate)
		{
			if(shaderTypes & c)
			{
				VkShaderStageFlagBits bit;
				if(ShaderTypeFlagToVkShaderStageFlagBits(c, bit))
				{
					flags |= bit;
				}
				else
				{
					return false;
				}
			}
		}
		return true;
	}

	bool PopulateInputBindingDescription(const VertexInputDetail* pData, uint32_t nCount, VulkanBindingDetailList& detailList)
	{
		using KVertexDefinition::VertexDetail;
		using KVertexDefinition::VertexSemanticDetail;
		using KVertexDefinition::VertexSemanticDetailList;

		if(pData == nullptr || nCount == 0)
			return false;

		detailList.clear();
		for(uint32_t idx = 0; idx < nCount; ++idx)
		{
			const VertexInputDetail& detail = pData[idx];

			for(size_t i = 0;i < detail.count; ++i)
			{
				VertexFormat format = detail.formats[i];
				VulkanBindingDetail bindingDetail;

				VkVertexInputBindingDescription& bindingDescription = bindingDetail.bindingDescription;				
				std::vector<VkVertexInputAttributeDescription>& attributeDescriptions = bindingDetail.attributeDescriptions;

				const VertexDetail& vertexDetail = KVertexDefinition::GetVertexDetail(format);

				// ����VkVertexInputBindingDescription
				bindingDescription.binding = idx;
				bindingDescription.stride = (uint32_t)vertexDetail.vertexSize;
				bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

				// ����VkVertexInputAttributeDescription
				const VertexSemanticDetailList& semanticDetailList = vertexDetail.semanticDetails;
				for(const VertexSemanticDetail& semanticDetail : semanticDetailList)
				{
					VkVertexInputAttributeDescription attributeDescription = {};
					VkFormat vkFormat = VK_FORMAT_UNDEFINED;
					ElementFormatToVkFormat(semanticDetail.elementFormat, vkFormat);

					attributeDescription.binding = idx;
					// ����ö��ֵ��Ϊ��λ��
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

	void CopyVkBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
	{
		VkCommandBuffer commandBuffer;
		BeginSingleTimeCommand(KVulkanGlobal::graphicsCommandPool, commandBuffer);
		{
			VkBufferCopy copyRegion = {};
			copyRegion.srcOffset = 0; // Optional
			copyRegion.dstOffset = 0; // Optional
			copyRegion.size = size;
			vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
		}
		EndSingleTimeCommand(KVulkanGlobal::graphicsQueue, KVulkanGlobal::graphicsCommandPool, commandBuffer);
	}

	void CopyVkBufferToVkImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
	{
		VkCommandBuffer commandBuffer;
		BeginSingleTimeCommand(KVulkanGlobal::graphicsCommandPool, commandBuffer);
		{
			VkBufferImageCopy region = {};
			region.bufferOffset = 0;
			// Ϊÿ��padding���� ����Ϊ0�Ժ���
			region.bufferRowLength = 0;
			// Ϊÿ��padding���� ����Ϊ0�Ժ���
			region.bufferImageHeight = 0;

			// VkImageSubresourceRange
			{
				// ���ﲻ����������mipmap
				region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				region.imageSubresource.mipLevel = 0;
				region.imageSubresource.baseArrayLayer = 0;
				region.imageSubresource.layerCount = 1;
			}

			VkOffset3D imageOffset = {0, 0, 0};
			region.imageOffset = imageOffset;

			VkExtent3D imageExtent = {width, height, 1};
			region.imageExtent = imageExtent;

			vkCmdCopyBufferToImage(
				commandBuffer,
				buffer,
				image,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1,
				&region
				);
		}
		EndSingleTimeCommand(KVulkanGlobal::graphicsQueue, KVulkanGlobal::graphicsCommandPool, commandBuffer);
	}

	void TransitionImageLayout(VkImage image, VkFormat format, uint32_t mipLevels, VkImageLayout oldLayout, VkImageLayout newLayout)
	{
		VkCommandBuffer commandBuffer;
		BeginSingleTimeCommand(KVulkanGlobal::graphicsCommandPool, commandBuffer);
		{
			VkImageMemoryBarrier barrier = {};
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			// ָ����Layout����Layout
			barrier.oldLayout = oldLayout;
			barrier.newLayout = newLayout;
			// ���ﲻ�������м�������Ȩת��
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

			barrier.image = image;
			// VkImageSubresourceRange
			{
				// ���ﲻ����������mipmap
				if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
				{
					barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

					if (HasStencilComponent(format))
					{
						barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
					}
				}
				else
				{
					barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				}
				barrier.subresourceRange.baseMipLevel = 0;
				barrier.subresourceRange.levelCount = mipLevels;
				barrier.subresourceRange.baseArrayLayer = 0;
				barrier.subresourceRange.layerCount = 1;
			}

			VkPipelineStageFlags sourceStage = 0;
			VkPipelineStageFlags destinationStage = 0;
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = 0;

			if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
			{
				barrier.srcAccessMask = 0;
				barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

				sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
				destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			}
			else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
			{
				barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

				sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
				destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			}
			else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
			{
				barrier.srcAccessMask = 0;
				barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

				sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
				destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
			}
			else if	(oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
			{
				barrier.srcAccessMask = 0;
				barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

				sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
				destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			}
			else
			{
				assert(false && "unsupported layout transition!");
			}

			vkCmdPipelineBarrier(
				commandBuffer,
				sourceStage, /* srcStageMask */
				destinationStage, /* dstStageMask */
				0, /* dependencyFlags �ò���Ϊ 0 ���� VK_DEPENDENCY_BY_REGION_BIT */
				0, nullptr,
				0, nullptr,
				1, &barrier
				);
		}
		EndSingleTimeCommand(KVulkanGlobal::graphicsQueue, KVulkanGlobal::graphicsCommandPool, commandBuffer);
	}

	void GenerateMipmaps(VkImage image, VkFormat format, int32_t texWidth, int32_t texHeight, uint32_t mipLevels)
	{
		// ����format�Ƿ�֧�����Թ���
		VkFormatProperties formatProperties;
		vkGetPhysicalDeviceFormatProperties(KVulkanGlobal::physicalDevice, format, &formatProperties);
		if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
		{
			ASSERT_RESULT(false && "Linear filter not supported on this format") ;
			return;
		}

		VkCommandBuffer commandBuffer;
		BeginSingleTimeCommand(KVulkanGlobal::graphicsCommandPool, commandBuffer);
		{
			VkImageMemoryBarrier barrier = {};
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.image = image;
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			barrier.subresourceRange.baseArrayLayer = 0;
			barrier.subresourceRange.layerCount = 1;
			barrier.subresourceRange.levelCount = 1;

			int32_t mipWidth = texWidth;
			int32_t mipHeight = texHeight;

			for (uint32_t i = 1; i < mipLevels; i++)
			{
				// �����ڴ����϶���һ��mipmapִ��һ��Transition ��TransferĿ��ת����transferԴ
				barrier.subresourceRange.baseMipLevel = i - 1;
				barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
				barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

				vkCmdPipelineBarrier(commandBuffer,
					VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
					0, nullptr,
					0, nullptr,
					1, &barrier);

				VkImageBlit blit = {};

				VkOffset3D srcOffsets[] = {{0, 0, 0}, {mipWidth, mipHeight, 1}};
				blit.srcOffsets[0] = srcOffsets[0];
				blit.srcOffsets[1] = srcOffsets[1];

				blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				blit.srcSubresource.mipLevel = i - 1;
				blit.srcSubresource.baseArrayLayer = 0;
				blit.srcSubresource.layerCount = 1;

				VkOffset3D dstOffsets[] = {{0, 0, 0}, { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 }};
				blit.dstOffsets[0] = dstOffsets[0];
				blit.dstOffsets[1] = dstOffsets[1];

				blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				blit.dstSubresource.mipLevel = i;
				blit.dstSubresource.baseArrayLayer = 0;
				blit.dstSubresource.layerCount = 1;

				// ִ��һ��blit ����һ�� mipmap blit ����һ�� mipmap
				vkCmdBlitImage(commandBuffer,
					image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					1, &blit,
					// ָ��biltʱ��ʹ�����Թ���
					VK_FILTER_LINEAR);

				// �����ڴ����϶���һ��mipmapִ��һ��Transition ��TransferԴת����Shader�ɶ�
				barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
				barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
				barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

				vkCmdPipelineBarrier(commandBuffer,
					VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
					0, nullptr,
					0, nullptr,
					1, &barrier);

				if (mipWidth > 1) mipWidth /= 2;
				if (mipHeight > 1) mipHeight /= 2;
			}

			// ���������������һ��mipmapִ��Transition ��TransferĿ��ת����Shader�ɶ�
			barrier.subresourceRange.baseMipLevel = mipLevels - 1;
			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			vkCmdPipelineBarrier(commandBuffer,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
				0, nullptr,
				0, nullptr,
				1, &barrier);
		}
		EndSingleTimeCommand(KVulkanGlobal::graphicsQueue, KVulkanGlobal::graphicsCommandPool, commandBuffer);
	}

	void BeginSingleTimeCommand(VkCommandPool commandPool, VkCommandBuffer& commandBuffer)
	{
		VkCommandBufferAllocateInfo allocInfo = KVulkanInitializer::CommandBufferAllocateInfo(commandPool);

		VK_ASSERT_RESULT(vkAllocateCommandBuffers(KVulkanGlobal::device, &allocInfo, &commandBuffer));

		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		VK_ASSERT_RESULT(vkBeginCommandBuffer(commandBuffer, &beginInfo));
	}

	void EndSingleTimeCommand(VkQueue queue, VkCommandPool commandPool, VkCommandBuffer& commandBuffer)
	{
		using namespace KVulkanGlobal;

		VK_ASSERT_RESULT(vkEndCommandBuffer(commandBuffer));

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		VK_ASSERT_RESULT(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));
		VK_ASSERT_RESULT(vkQueueWaitIdle(queue));

		vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
	}

	bool FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features, VkFormat& vkFormat)
	{
		using namespace KVulkanGlobal;

		for (VkFormat format : candidates)
		{
			VkFormatProperties props;
			// ��ȡ��VkFormat��Ӧ��tProperties
			vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

			if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
			{
				vkFormat = format;
				return true;
			}
			else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
			{
				vkFormat = format;
				return true;
			}
		}

		return false;
	}

	bool HasStencilComponent(VkFormat format)
	{
		return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
	}

	bool QueryMSAASupport(MSAASupportTarget target, uint32_t msaaCount, VkSampleCountFlagBits& flag)
	{
		using namespace KVulkanGlobal;

		VkPhysicalDeviceProperties physicalDeviceProperties;
		vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);

		VkSampleCountFlags counts = 0;
		
		if(flag == MST_BOTH)
		{
			counts = std::min(physicalDeviceProperties.limits.framebufferColorSampleCounts, physicalDeviceProperties.limits.framebufferDepthSampleCounts);
		}
		else if(flag == MST_COLOR)
		{
			counts = physicalDeviceProperties.limits.framebufferColorSampleCounts;
		}
		else
		{
			counts = physicalDeviceProperties.limits.framebufferDepthSampleCounts;
		}

		flag = VK_SAMPLE_COUNT_FLAG_BITS_MAX_ENUM;


#define CHECK_ASSIGN_MSAA_FLAG(COUNT)\
case COUNT:\
{\
	flag = (counts & VK_SAMPLE_COUNT_##COUNT##_BIT) > 0 ? VK_SAMPLE_COUNT_##COUNT##_BIT : VK_SAMPLE_COUNT_FLAG_BITS_MAX_ENUM;\
	break;\
}

#define DEFAULT_ASSIGN_MSAA_FLAG()\
default:\
{\
	flag = VK_SAMPLE_COUNT_FLAG_BITS_MAX_ENUM;\
	break;\
}

		switch (msaaCount)
		{
			CHECK_ASSIGN_MSAA_FLAG(1);
			CHECK_ASSIGN_MSAA_FLAG(2);
			CHECK_ASSIGN_MSAA_FLAG(4);
			CHECK_ASSIGN_MSAA_FLAG(8);
			CHECK_ASSIGN_MSAA_FLAG(16);
			CHECK_ASSIGN_MSAA_FLAG(32);
			CHECK_ASSIGN_MSAA_FLAG(64);
			DEFAULT_ASSIGN_MSAA_FLAG();
		}

#undef DEFAULT_ASSIGN_MSAA_FLAG
#undef CHECK_ASSIGN_MSAA_FLAG

		return flag != VK_SAMPLE_COUNT_FLAG_BITS_MAX_ENUM;
	}
}