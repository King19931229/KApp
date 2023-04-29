#include "KVulkanPipelineLayout.h"
#include "KVulkanGlobal.h"

KVulkanPipelineLayout::KVulkanPipelineLayout()
	: m_DescriptorSetLayout(VK_NULL_HANDEL)
	, m_PipelineLayout(VK_NULL_HANDEL)
	, m_Hash(0)
{
}

KVulkanPipelineLayout::~KVulkanPipelineLayout()
{
	ASSERT_RESULT(m_DescriptorSetLayout == VK_NULL_HANDEL);
	ASSERT_RESULT(m_PipelineLayout == VK_NULL_HANDEL);
}

bool KVulkanPipelineLayout::Init(const KPipelineBinding& binding)
{
	UnInit();

	static VkShaderStageFlagBits SHADER_STAGE_FLAGS[] =
	{
		VK_SHADER_STAGE_VERTEX_BIT,
		VK_SHADER_STAGE_FRAGMENT_BIT,
		VK_SHADER_STAGE_GEOMETRY_BIT,
		VK_SHADER_STAGE_TASK_BIT_NV,
		VK_SHADER_STAGE_MESH_BIT_NV
	};

	static_assert(ARRAY_SIZE(SHADER_STAGE_FLAGS) == LAYOUT_SHADER_COUNT, "must match");

	m_Hash = 0;

	std::vector<VkPushConstantRange> pushConstantRanges;
	for (uint32_t i = 0; i < LAYOUT_SHADER_COUNT; ++i)
	{
		if (binding.shaders[i])
		{
			const KShaderInformation& info = binding.shaders[i]->GetInformation();
			AddLayoutBinding(info, SHADER_STAGE_FLAGS[i]);
			AddPushConstantRange(pushConstantRanges, info, SHADER_STAGE_FLAGS[i]);
			KHash::HashCombine(m_Hash, info.Hash());
		}
		else
		{
			KHash::HashCombine(m_Hash, 0);
		}
	}

	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(m_DescriptorSetLayoutBinding.size());
	layoutInfo.pBindings = m_DescriptorSetLayoutBinding.data();

	VK_ASSERT_RESULT(vkCreateDescriptorSetLayout(KVulkanGlobal::device, &layoutInfo, nullptr, &m_DescriptorSetLayout));

	// 创建管线布局
	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	// 指定该管线的描述布局
	pipelineLayoutInfo.pSetLayouts = &m_DescriptorSetLayout;
	// 指定PushConstant
	pipelineLayoutInfo.pushConstantRangeCount = static_cast<uint32_t>(pushConstantRanges.size());
	pipelineLayoutInfo.pPushConstantRanges = pushConstantRanges.size() > 0 ? pushConstantRanges.data() : nullptr;

	VK_ASSERT_RESULT(vkCreatePipelineLayout(KVulkanGlobal::device, &pipelineLayoutInfo, nullptr, &m_PipelineLayout));

	return true;
}

bool KVulkanPipelineLayout::UnInit()
{
	m_DescriptorSetLayoutBinding.clear();

	if (m_DescriptorSetLayout)
	{
		vkDestroyDescriptorSetLayout(KVulkanGlobal::device, m_DescriptorSetLayout, nullptr);
		m_DescriptorSetLayout = VK_NULL_HANDLE;
	}

	if (m_PipelineLayout)
	{
		vkDestroyPipelineLayout(KVulkanGlobal::device, m_PipelineLayout, nullptr);
		m_PipelineLayout = VK_NULL_HANDLE;
	}

	m_Hash = 0;

	return true;
}

void KVulkanPipelineLayout::MergeLayoutBinding(std::vector<VkDescriptorSetLayoutBinding>& bindings, const VkDescriptorSetLayoutBinding& newBinding)
{
	auto it = std::find_if(bindings.begin(), bindings.end(),
		[&newBinding](const VkDescriptorSetLayoutBinding& reference)->bool
		{
			if (newBinding.binding != reference.binding)
				return false;
			if (newBinding.descriptorType != reference.descriptorType)
				return false;
			if (newBinding.descriptorCount != reference.descriptorCount)
				return false;
			if (newBinding.pImmutableSamplers != reference.pImmutableSamplers)
				return false;
			return true;
		});

	if (it == bindings.end())
	{
		bindings.push_back(newBinding);
	}
	else
	{
		(*it).stageFlags |= newBinding.stageFlags;
	}
}

void KVulkanPipelineLayout::AddLayoutBinding(const KShaderInformation& information, VkShaderStageFlags stageFlag)
{
	for (const KShaderInformation::Storage& storage : information.storageBuffers)
	{
		VkDescriptorSetLayoutBinding sboLayoutBinding = {};
		// 与Shader中绑定位置对应
		sboLayoutBinding.binding = storage.bindingIndex;
		sboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		sboLayoutBinding.descriptorCount = storage.arraysize > 1 ? storage.arraysize : 1;
		// 声明哪个阶段Shader能够使用上此UBO
		sboLayoutBinding.stageFlags = stageFlag;
		sboLayoutBinding.pImmutableSamplers = nullptr; // Optional

		MergeLayoutBinding(m_DescriptorSetLayoutBinding, sboLayoutBinding);
	}

	for (const KShaderInformation::Storage& storage : information.storageImages)
	{
		VkDescriptorSetLayoutBinding sboLayoutBinding = {};
		// 与Shader中绑定位置对应
		sboLayoutBinding.binding = storage.bindingIndex;
		sboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		sboLayoutBinding.descriptorCount = storage.arraysize > 1 ? storage.arraysize : 1;
		// 声明哪个阶段Shader能够使用上此UBO
		sboLayoutBinding.stageFlags = stageFlag;
		sboLayoutBinding.pImmutableSamplers = nullptr; // Optional

		MergeLayoutBinding(m_DescriptorSetLayoutBinding, sboLayoutBinding);
	}

	for (const KShaderInformation::Constant& constant : information.constants)
	{
		VkDescriptorSetLayoutBinding uboLayoutBinding = {};
		// 与Shader中绑定位置对应
		uboLayoutBinding.binding = constant.bindingIndex;
		uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uboLayoutBinding.descriptorCount = 1;
		// 声明哪个阶段Shader能够使用上此UBO
		uboLayoutBinding.stageFlags = stageFlag;
		uboLayoutBinding.pImmutableSamplers = nullptr; // Optional

		MergeLayoutBinding(m_DescriptorSetLayoutBinding, uboLayoutBinding);
	}

	for (const KShaderInformation::Constant& constant : information.dynamicConstants)
	{
		VkDescriptorSetLayoutBinding uboLayoutBinding = {};
		// 与Shader中绑定位置对应
		uboLayoutBinding.binding = constant.bindingIndex;
		uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		uboLayoutBinding.descriptorCount = 1;
		// 声明哪个阶段Shader能够使用上此UBO
		uboLayoutBinding.stageFlags = stageFlag;
		uboLayoutBinding.pImmutableSamplers = nullptr; // Optional

		MergeLayoutBinding(m_DescriptorSetLayoutBinding, uboLayoutBinding);
	}

	for (const KShaderInformation::Texture& texture : information.textures)
	{
		VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
		// 与Shader中绑定位置对应
		samplerLayoutBinding.binding = texture.bindingIndex;
		samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		samplerLayoutBinding.descriptorCount = texture.arraysize > 1 ? texture.arraysize : 1;
		// 声明哪个阶段Shader能够使用上此Sampler
		samplerLayoutBinding.stageFlags = stageFlag;
		samplerLayoutBinding.pImmutableSamplers = nullptr; // Optional

		MergeLayoutBinding(m_DescriptorSetLayoutBinding, samplerLayoutBinding);
	}
}

void KVulkanPipelineLayout::AddPushConstantRange(std::vector<VkPushConstantRange>& ranges, const KShaderInformation& information, VkShaderStageFlags stageFlag)
{
	for (const KShaderInformation::Constant& constant : information.pushConstants)
	{
		VkPushConstantRange range = {};
		range.stageFlags = stageFlag;
		range.offset = 0;
		range.size = constant.size;

		auto it = std::find_if(ranges.begin(), ranges.end(),
			[&range](const VkPushConstantRange& reference)->bool
			{
				if (range.offset != reference.offset)
					return false;
				if (range.size != reference.size)
					return false;
				return true;
			});

		if (it == ranges.end())
		{
			ranges.push_back(range);
		}
		else
		{
			(*it).stageFlags |= range.stageFlags;
		}
	}
}