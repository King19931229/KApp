#include "KVulkanHelper.h"
#include "KVulkanGlobal.h"
#include "KVulkanInitializer.h"
#include <algorithm>

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

	bool TextureTypeToVkImageType(TextureType textureType, VkImageType& imageType, VkImageViewType& imageViewType)
	{
		switch (textureType)
		{
		case TT_TEXTURE_2D:
			imageType = VK_IMAGE_TYPE_2D;
			imageViewType = VK_IMAGE_VIEW_TYPE_2D;
			return true;
		case TT_TEXTURE_3D:
			imageType = VK_IMAGE_TYPE_3D;
			imageViewType = VK_IMAGE_VIEW_TYPE_3D;
			return true;
		case TT_TEXTURE_CUBE_MAP:
			imageType = VK_IMAGE_TYPE_2D;
			imageViewType = VK_IMAGE_VIEW_TYPE_CUBE;
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
		case EF_R8G8_UNORM:
			vkFormat = VK_FORMAT_R8G8_UNORM;
			return true;
		case EF_R8_UNORM:
			vkFormat = VK_FORMAT_R8_UNORM;
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

		case EF_R32G32B32A32_UINT:
			vkFormat = VK_FORMAT_R32G32B32A32_UINT;
			return true;

		case EF_ETC1_R8G8B8_UNORM:
			// TODO Correct?
			vkFormat = VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK;
			return true;
		case EF_ETC2_R8G8B8_UNORM:
			vkFormat = VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK;
			return true;
		case EF_ETC2_R8G8B8A1_UNORM:
			vkFormat = VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK;
			return true;
		case EF_ETC2_R8G8B8A8_UNORM:
			vkFormat = VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK;
			return true;

		case EF_BC1_RGB_UNORM:
			vkFormat = VK_FORMAT_BC1_RGB_UNORM_BLOCK;
			return true;
		case EF_BC1_RGB_SRGB:
			vkFormat = VK_FORMAT_BC1_RGB_SRGB_BLOCK;
			return true;
		case EF_BC1_RGBA_UNORM:
			vkFormat = VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
			return true;
		case EF_BC1_RGBA_SRGB:
			vkFormat = VK_FORMAT_BC1_RGBA_SRGB_BLOCK;
			return true;
		case EF_BC2_UNORM:
			vkFormat = VK_FORMAT_BC2_UNORM_BLOCK;
			return true;
		case EF_BC2_SRGB:
			vkFormat = VK_FORMAT_BC2_SRGB_BLOCK;
			return true;
		case EF_BC3_UNORM:
			vkFormat = VK_FORMAT_BC3_UNORM_BLOCK;
			return true;
		case EF_BC3_SRGB:
			vkFormat = VK_FORMAT_BC3_SRGB_BLOCK;
			return true;
		case EF_BC4_UNORM:
			vkFormat = VK_FORMAT_BC4_UNORM_BLOCK;
			return true;
		case EF_BC4_SNORM:
			vkFormat = VK_FORMAT_BC4_SNORM_BLOCK;
			return true;
		case EF_BC5_UNORM:
			vkFormat = VK_FORMAT_BC5_UNORM_BLOCK;
			return true;
		case EF_BC5_SNORM:
			vkFormat = VK_FORMAT_BC5_SNORM_BLOCK;
			return true;
		case EF_BC6H_UFLOAT:
			vkFormat = VK_FORMAT_BC6H_UFLOAT_BLOCK;
			return true;
		case EF_BC6H_SFLOAT:
			vkFormat = VK_FORMAT_BC6H_SFLOAT_BLOCK;
			return true;
		case EF_BC7_UNORM:
			vkFormat = VK_FORMAT_BC7_UNORM_BLOCK;
			return true;
		case EF_BC7_SRGB:
			vkFormat = VK_FORMAT_BC7_SRGB_BLOCK;
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
		case AM_CLAMP_TO_EDGE:
			vkAddressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
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
		case PT_POINT_LIST:
			vkPrimitiveTopology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
			return true;
		case PT_TRIANGLE_LIST:
			vkPrimitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
			return true;
		case PT_TRIANGLE_STRIP:
			vkPrimitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
			return true;
		case PT_LINE_LIST:
			vkPrimitiveTopology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
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

	bool CompareFuncToVkCompareOp(CompareFunc func, VkCompareOp& op)
	{
		switch (func)
		{
		case CF_NEVER:
			op = VK_COMPARE_OP_NEVER;
			return true;
		case CF_LESS:
			op = VK_COMPARE_OP_LESS;
			return true;
		case CF_EQUAL:
			op = VK_COMPARE_OP_EQUAL;
			return true;
		case CF_LESS_OR_EQUAL:
			op = VK_COMPARE_OP_LESS_OR_EQUAL;
			return true;
		case CF_GREATER:
			op = VK_COMPARE_OP_GREATER;
			return true;
		case CF_NOT_EQUAL:
			op = VK_COMPARE_OP_NOT_EQUAL;
			return true;
		case CF_GREATER_OR_EQUAL:
			op = VK_COMPARE_OP_GREATER_OR_EQUAL;
			return true;
		case CF_ALWAYS:
			op = VK_COMPARE_OP_ALWAYS;
			return true;
		default:
			assert(false && "compare func not supported");
			return false;
		}
	}

	bool StencilOperatorToVkStencilOp(StencilOperator stencilOperator, VkStencilOp& vkStencilOp)
	{
		switch (stencilOperator)
		{
		case SO_KEEP:
			vkStencilOp = VK_STENCIL_OP_KEEP;
			return true;
		case SO_ZERO:
			vkStencilOp = VK_STENCIL_OP_ZERO;
			return true;
		case SO_REPLACE:
			vkStencilOp = VK_STENCIL_OP_REPLACE;
			return true;
		case SO_INC:
			vkStencilOp = VK_STENCIL_OP_INCREMENT_AND_CLAMP;
			return true;
		case SO_DEC:
			vkStencilOp = VK_STENCIL_OP_DECREMENT_AND_CLAMP;
			return true;
		default:
			assert(false && "stencil operator not supported");
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
		case BF_SRC_ALPHA:
			vkBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
			return true;
		case BF_ONE_MINUS_SRC_ALPHA:
			vkBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
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

	bool LoadOpToVkAttachmentLoadOp(LoadOperation op, VkAttachmentLoadOp& vkLoadOp)
	{
		switch (op)
		{
			case LO_LOAD:
				vkLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
				return true;
			case LO_DONT_CARE:
				vkLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
				return true;
			case LO_CLEAR:
				vkLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
				return true;
			default:
				assert(false && "load operation not supported");
				return false;
		}
	}

	bool StoreOpToVkAttachmentStoreOp(StoreOperation op, VkAttachmentStoreOp& vkStoreOp)
	{
		switch (op)
		{
			case SO_STORE:
				vkStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
				return true;
			case SO_DONT_CARE:
				vkStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
				return true;
			default:
				assert(false && "load operation not supported");
				return false;
		}
	}

	bool QueryTypeToVkQueryType(QueryType queryType, VkQueryType& vkQueryType)
	{
		switch (queryType)
		{
		case QT_OCCLUSION:
			vkQueryType = VK_QUERY_TYPE_OCCLUSION;
			return true;
		default:
			assert(false && "queryType not supported");
			return false;
		}
	}

	bool ShaderTypeFlagToVkShaderStageFlagBits(ShaderType shaderTypeFlag, VkShaderStageFlagBits& bit)
	{
		switch (shaderTypeFlag)
		{
		case ST_VERTEX:
			bit = VK_SHADER_STAGE_VERTEX_BIT;
			return true;
		case ST_FRAGMENT:
			bit = VK_SHADER_STAGE_FRAGMENT_BIT;
			return true;
		case ST_GEOMETRY:
			bit = VK_SHADER_STAGE_GEOMETRY_BIT;
			return true;
		case ST_RAYGEN:
			bit = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
			return true;
		case ST_ANY_HIT:
			bit = VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
			return true;
		case ST_CLOSEST_HIT:
			bit = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
			return true;
		case ST_MISS:
			bit = VK_SHADER_STAGE_MISS_BIT_KHR;
			return true;
		case ST_COMPUTE:
			bit = VK_SHADER_STAGE_COMPUTE_BIT;
			return true;
		default:
			assert(false && "Unknown shader type flag");
			return false;
		}
	}

	bool ShaderTypesToVkShaderStageFlag(ShaderTypes shaderTypes, VkFlags& flags)
	{
		flags = 0;
		const ShaderType candidate[] = { ST_VERTEX, ST_FRAGMENT, ST_GEOMETRY, ST_RAYGEN, ST_ANY_HIT, ST_CLOSEST_HIT , ST_MISS, ST_COMPUTE, ST_TASK, ST_MESH };
		static_assert((1 << (ARRAY_SIZE(candidate) - 1)) + 1 == ST_ENDENUM, "Array size should match");
		for(ShaderType c : candidate)
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

	bool ImageLayoutToVkImageLayout(ImageLayout layout, VkImageLayout vkImageLayout)
	{
		switch (layout)
		{
			case IMAGE_LAYOUT_GENERAL:
				vkImageLayout = VK_IMAGE_LAYOUT_GENERAL;
				return true;
			case IMAGE_LAYOUT_COLOR_ATTACHMENT:
				vkImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
				return true;
			case IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT:
				vkImageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
				return true;
			case IMAGE_LAYOUT_SHADER_READ_ONLY:
				vkImageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				return true;
			case IMAGE_LAYOUT_UNDEFINED:
				vkImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				return true;
			default:
				assert(false && "Unknown image layout");
				return false;
		}
	}

	bool PopulateInputBindingDescription(const VertexFormat* pData, size_t uCount, VulkanBindingDetailList& detailList)
	{
		using KVertexDefinition::VertexDetail;
		using KVertexDefinition::VertexSemanticDetail;
		using KVertexDefinition::VertexSemanticDetailList;

		if(pData == nullptr || uCount == 0)
			return false;

		detailList.clear();
		for(size_t idx = 0; idx < uCount; ++idx)
		{
			VertexFormat format = pData[idx];

			VulkanBindingDetail bindingDetail = {};

			VkVertexInputBindingDescription& bindingDescription = bindingDetail.bindingDescription;				
			std::vector<VkVertexInputAttributeDescription>& attributeDescriptions = bindingDetail.attributeDescriptions;

			const VertexDetail& vertexDetail = KVertexDefinition::GetVertexDetail(format);

			assert(vertexDetail.vertexSize > 0 && "impossible to get a zero size vertex");

			// 构造VkVertexInputBindingDescription
			bindingDescription.binding = static_cast<uint32_t>(idx);

			bindingDescription.stride = (uint32_t)vertexDetail.vertexSize;
			bindingDescription.inputRate = (format != VF_INSTANCE) ? VK_VERTEX_INPUT_RATE_VERTEX : VK_VERTEX_INPUT_RATE_INSTANCE;

			// 构造VkVertexInputAttributeDescription
			const VertexSemanticDetailList& semanticDetailList = vertexDetail.semanticDetails;
			for(const VertexSemanticDetail& semanticDetail : semanticDetailList)
			{
				VkVertexInputAttributeDescription attributeDescription = {};
				VkFormat vkFormat = VK_FORMAT_UNDEFINED;
				ElementFormatToVkFormat(semanticDetail.elementFormat, vkFormat);

				attributeDescription.binding = static_cast<uint32_t>(idx);
				// 语意枚举值即为绑定位置
				attributeDescription.location = semanticDetail.semantic;
				attributeDescription.format = vkFormat;
				attributeDescription.offset = (uint32_t)semanticDetail.offset;

				attributeDescriptions.push_back(attributeDescription);
			}

			detailList.push_back(bindingDetail);
		}

		return true;
	}

	bool FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features, VkFormat& vkFormat)
	{
		using namespace KVulkanGlobal;

		for (VkFormat format : candidates)
		{
			VkFormatProperties props;
			// 获取该VkFormat对应的tProperties
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

	bool FindBestDepthFormat(bool bStencil, VkFormat& format)
	{
		format = VK_FORMAT_MAX_ENUM;
		std::vector<VkFormat> candidates;

		if (!bStencil)
		{
			candidates.push_back(VK_FORMAT_D32_SFLOAT);
		}
		candidates.push_back(VK_FORMAT_D32_SFLOAT_S8_UINT);
		candidates.push_back(VK_FORMAT_D24_UNORM_S8_UINT);
		candidates.push_back(VK_FORMAT_D16_UNORM_S8_UINT);

		ASSERT_RESULT(KVulkanHelper::FindSupportedFormat(candidates, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT, format));

		return true;
	}

	bool QueryMSAASupport(MSAASupportTarget target, uint32_t msaaCount, VkSampleCountFlagBits& flag)
	{
		using namespace KVulkanGlobal;

		VkPhysicalDeviceProperties physicalDeviceProperties;
		vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);

		VkSampleCountFlags counts = 0;
		
		if(target == MST_BOTH)
		{
			counts = std::min(physicalDeviceProperties.limits.framebufferColorSampleCounts, physicalDeviceProperties.limits.framebufferDepthSampleCounts);
		}
		else if(target == MST_COLOR)
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

	bool GetBufferDeviceAddress(VkBuffer buffer, VkDeviceAddress& address)
	{
		VkBufferDeviceAddressInfoKHR bufferDeviceAI{};
		bufferDeviceAI.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
		bufferDeviceAI.buffer = buffer;
		address = KVulkanGlobal::vkGetBufferDeviceAddressKHR(KVulkanGlobal::device, &bufferDeviceAI);
		return true;
	}
}