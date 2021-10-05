#include "KVulkanPipeline.h"
#include "KVulkanBuffer.h"
#include "KVulkanTexture.h"
#include "KVulkanSampler.h"
#include "KVulkanHelper.h"
#include "KVulkanShader.h"
#include "KVulkanRenderTarget.h"
#include "KVulkanRenderPass.h"
#include "KVulkanFrameBuffer.h"
#include "KVulkanGlobal.h"

static VkShaderStageFlagBits SHADER_STAGE_FLAGS[] =
{
	VK_SHADER_STAGE_VERTEX_BIT,
	VK_SHADER_STAGE_FRAGMENT_BIT,
	VK_SHADER_STAGE_TASK_BIT_NV,
	VK_SHADER_STAGE_MESH_BIT_NV
};

static_assert(ARRAY_SIZE(SHADER_STAGE_FLAGS) == KVulkanPipeline::COUNT, "size must match");

KVulkanPipeline::KVulkanPipeline() :
	m_PrimitiveTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST),
	m_ColorWriteMask(VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT),
	m_ColorSrcBlendFactor(VK_BLEND_FACTOR_SRC_ALPHA),
	m_ColorDstBlendFactor(VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA),
	m_ColorBlendOp(VK_BLEND_OP_ADD),
	m_BlendEnable(VK_FALSE),
	m_PolygonMode(VK_POLYGON_MODE_FILL),
	m_CullMode(VK_CULL_MODE_BACK_BIT),
	m_FrontFace(VK_FRONT_FACE_COUNTER_CLOCKWISE),
	m_DepthWrite(VK_TRUE),
	m_DepthTest(VK_TRUE),
	m_DepthCompareOp(VK_COMPARE_OP_LESS_OR_EQUAL),
	m_DescriptorSetLayout(VK_NULL_HANDLE),
	m_PipelineLayout(VK_NULL_HANDLE),
	m_DepthBiasEnable(VK_FALSE),
	m_StencilFailOp(VK_STENCIL_OP_KEEP),
	m_StencilDepthFailOp(VK_STENCIL_OP_KEEP),
	m_StencilPassOp(VK_STENCIL_OP_KEEP),
	m_StencilCompareOp(VK_COMPARE_OP_ALWAYS),
	m_StencilRef(0),
	m_StencilEnable(VK_FALSE)
{
	m_PushContant = { 0, 0 };

	m_RenderPassInvalidCB = [this](IKRenderPass* renderPass)
	{
		InvaildHandle(renderPass);
	};
}

KVulkanPipeline::~KVulkanPipeline()
{
	ASSERT_RESULT(m_DescriptorSetLayout == VK_NULL_HANDLE);
	ASSERT_RESULT(m_PipelineLayout == VK_NULL_HANDLE);
	for (uint32_t i = 0; i < COUNT; ++i)
	{
		ASSERT_RESULT(m_Shaders[i] == nullptr);
	}
	ASSERT_RESULT(m_Uniforms.empty());
	ASSERT_RESULT(m_Samplers.empty());
}

bool KVulkanPipeline::InvaildHandle(IKRenderPass* renderPass)
{
	if (renderPass)
	{
		auto it = m_HandleMap.find(renderPass);
		if (it != m_HandleMap.end())
		{
			IKPipelineHandlePtr handle = it->second;
			handle->UnInit();
			m_HandleMap.erase(it);
		}
	}
	return true;
}

bool KVulkanPipeline::DestroyDevice()
{
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

	return true;
}

bool KVulkanPipeline::ClearHandle()
{
	for (auto& pair : m_HandleMap)
	{
		IKRenderPass* pass = pair.first;
		IKPipelineHandlePtr handle = pair.second;
		pass->UnRegisterInvalidCallback(&m_RenderPassInvalidCB);
		handle->UnInit();
	}
	m_HandleMap.clear();

	return true;
}

bool KVulkanPipeline::SetPrimitiveTopology(PrimitiveTopology topology)
{
	ASSERT_RESULT(KVulkanHelper::TopologyToVkPrimitiveTopology(topology, m_PrimitiveTopology));
	return true;
}

bool KVulkanPipeline::SetVertexBinding(const VertexFormat* formats, size_t count)
{
	KVulkanHelper::VulkanBindingDetailList bindingDetails;
	ASSERT_RESULT(KVulkanHelper::PopulateInputBindingDescription(formats, count, bindingDetails));

	for(KVulkanHelper::VulkanBindingDetail& detail : bindingDetails)
	{
		m_BindingDescriptions.push_back(detail.bindingDescription);
		m_AttributeDescriptions.insert(m_AttributeDescriptions.end(),
			detail.attributeDescriptions.begin(),
			detail.attributeDescriptions.end()
			);
	}
	return true;
}

bool KVulkanPipeline::SetColorWrite(bool r, bool g, bool b, bool a)
{
	m_ColorWriteMask = 0;
	m_ColorWriteMask |=  (r * VK_COLOR_COMPONENT_R_BIT);
	m_ColorWriteMask |=  (g * VK_COLOR_COMPONENT_G_BIT);
	m_ColorWriteMask |=  (b * VK_COLOR_COMPONENT_B_BIT);
	m_ColorWriteMask |=  (a * VK_COLOR_COMPONENT_A_BIT);
	return true;
}

bool KVulkanPipeline::SetColorBlend(BlendFactor srcFactor, BlendFactor dstFactor, BlendOperator op)
{
	ASSERT_RESULT(KVulkanHelper::BlendFactorToVkBlendFactor(srcFactor, m_ColorSrcBlendFactor));
	ASSERT_RESULT(KVulkanHelper::BlendFactorToVkBlendFactor(dstFactor, m_ColorDstBlendFactor));
	ASSERT_RESULT(KVulkanHelper::BlendOperatorToVkBlendOp(op, m_ColorBlendOp));
	return true;
}

bool KVulkanPipeline::SetBlendEnable(bool enable)
{
	m_BlendEnable = enable;
	return true;
}

bool KVulkanPipeline::SetCullMode(CullMode cullMode)
{
	ASSERT_RESULT(KVulkanHelper::CullModeToVkCullMode(cullMode, m_CullMode));
	return true;
}

bool KVulkanPipeline::SetFrontFace(FrontFace frontFace)
{
	ASSERT_RESULT(KVulkanHelper::FrontFaceToVkFrontFace(frontFace, m_FrontFace));
	return true;
}

bool KVulkanPipeline::SetPolygonMode(PolygonMode polygonMode)
{
	ASSERT_RESULT(KVulkanHelper::PolygonModeToVkPolygonMode(polygonMode, m_PolygonMode));
	return true;
}

bool KVulkanPipeline::SetDepthFunc(CompareFunc func, bool depthWrtie, bool depthTest)
{
	ASSERT_RESULT(KVulkanHelper::CompareFuncToVkCompareOp(func, m_DepthCompareOp));
	m_DepthWrite = (VkBool32)depthWrtie;
	m_DepthTest = (VkBool32)depthTest;
	return true;
}

bool KVulkanPipeline::SetDepthBiasEnable(bool enable)
{
	m_DepthBiasEnable = (VkBool32)enable;
	return true;
}

bool KVulkanPipeline::SetStencilFunc(CompareFunc func, StencilOperator failOp, StencilOperator depthFailOp, StencilOperator passOp)
{
	ASSERT_RESULT(KVulkanHelper::CompareFuncToVkCompareOp(func, m_StencilCompareOp));	
	ASSERT_RESULT(KVulkanHelper::StencilOperatorToVkStencilOp(failOp, m_StencilFailOp));
	ASSERT_RESULT(KVulkanHelper::StencilOperatorToVkStencilOp(depthFailOp, m_StencilDepthFailOp));
	ASSERT_RESULT(KVulkanHelper::StencilOperatorToVkStencilOp(passOp, m_StencilPassOp));
	return true;
}

bool KVulkanPipeline::SetStencilRef(uint32_t ref)
{
	m_StencilRef = ref;
	return true;
}

bool KVulkanPipeline::SetStencilEnable(bool enable)
{
	m_StencilEnable = (VkBool32)enable;
	return true;
}

bool KVulkanPipeline::SetShader(ShaderType shaderType, IKShaderPtr shader)
{
	switch (shaderType)
	{
	case ST_VERTEX:
		m_Shaders[VERTEX] = shader;
		return true;
	case ST_FRAGMENT:
		m_Shaders[FRAGMENT] = shader;
		return true;
	case ST_TASK:
		m_Shaders[TASK] = shader;
		return true;
	case ST_MESH:
		m_Shaders[MESH] = shader;
		return true;
	default:
		assert(false && "unknown shader");
		return false;
	}
}

bool KVulkanPipeline::SetConstantBuffer(unsigned int location, ShaderTypes shaderTypes, IKUniformBufferPtr buffer)
{
	if(buffer)
	{
		ASSERT_RESULT(m_Samplers.find(location) == m_Samplers.end() && "The location you try to bind is conflited with sampler");

		UniformBufferBindingInfo info = { shaderTypes, buffer };
		auto it = m_Uniforms.find(location);
		if(it == m_Uniforms.end())
		{
			m_Uniforms[location] = info;
		}
		else
		{
			it->second = info;
		}
		return true;
	}
	return false;
}

bool KVulkanPipeline::BindSampler(unsigned int location, const SamplerBindingInfo& info)
{
	ASSERT_RESULT(m_Uniforms.find(location) == m_Uniforms.end() && "The location you try to bind is conflited with ubo");

	auto it = m_Samplers.find(location);
	if(it == m_Samplers.end())
	{
		m_Samplers[location] = info;
	}
	else
	{
		it->second = info;
	}

	return true;
}

bool KVulkanPipeline::SetSampler(unsigned int location, IKTexturePtr texture, IKSamplerPtr sampler, bool dynimicWrite)
{
	ASSERT_RESULT(texture && sampler);
	if (texture && sampler)
	{
		SamplerBindingInfo info;
		info.texture = texture;
		info.sampler = sampler;
		info.dynamicWrite = dynimicWrite;
		info.onceWrite = false;
		ASSERT_RESULT(BindSampler(location, info));
		return true;
	}
	return false;
}

bool KVulkanPipeline::SetSampler(unsigned int location, IKRenderTargetPtr target, IKSamplerPtr sampler, bool dynimicWrite)
{
	if (target && sampler)
	{
		SamplerBindingInfo info;
		info.target = target;
		info.sampler = sampler;
		info.dynamicWrite = dynimicWrite;
		info.onceWrite = false;
		ASSERT_RESULT(BindSampler(location, info));
		return true;
	}
	return false;
}

bool KVulkanPipeline::CreateConstantBlock(ShaderTypes shaderTypes, uint32_t size)
{
	m_PushContant = { shaderTypes, size };
	return true;
}

bool KVulkanPipeline::DestroyConstantBlock()
{
	m_PushContant = { 0, 0 };
	return true;
}

bool KVulkanPipeline::CreateLayout()
{
	ASSERT_RESULT(m_DescriptorSetLayout == VK_NULL_HANDLE);
	ASSERT_RESULT(m_PipelineLayout == VK_NULL_HANDLE);

	/*
	DescriptorSetLayout 仅仅声明UBO Sampler绑定的位置
	实际UBO Sampler 句柄绑定在描述集合里指定
	*/
	m_DescriptorSetLayoutBinding.clear();

	auto MergeLayoutBinding = [](std::vector<VkDescriptorSetLayoutBinding>& bindings, const VkDescriptorSetLayoutBinding& newBinding)
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
	};

	auto AddLayoutBinding = [this, MergeLayoutBinding](const KShaderInformation& information, VkShaderStageFlags stageFlag)
	{
		for (const KShaderInformation::Storage& storage : information.storages)
		{
			VkDescriptorSetLayoutBinding sboLayoutBinding = {};
			// 与Shader中绑定位置对应
			sboLayoutBinding.binding = storage.bindingIndex;
			sboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			// 声明SBO Buffer数组长度 这里不使用数组
			sboLayoutBinding.descriptorCount = 1;
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
			// 声明UBO Buffer数组长度 这里不使用数组
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
			// 声明UBO Buffer数组长度 这里不使用数组
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
			// 这里不使用数组
			samplerLayoutBinding.descriptorCount = 1;
			// 声明哪个阶段Shader能够使用上此Sampler
			samplerLayoutBinding.stageFlags = stageFlag;
			samplerLayoutBinding.pImmutableSamplers = nullptr; // Optional

			MergeLayoutBinding(m_DescriptorSetLayoutBinding, samplerLayoutBinding);
		}
	};

	for (uint32_t i = 0; i < COUNT; ++i)
	{
		if (m_Shaders[i])
		{
			AddLayoutBinding(m_Shaders[i]->GetInformation(), SHADER_STAGE_FLAGS[i]);
		}
	}

	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(m_DescriptorSetLayoutBinding.size());
	layoutInfo.pBindings = m_DescriptorSetLayoutBinding.data();

	VK_ASSERT_RESULT(vkCreateDescriptorSetLayout(KVulkanGlobal::device, &layoutInfo, nullptr, &m_DescriptorSetLayout));

	/*
	声明PushConstant
	*/
	std::vector<VkPushConstantRange> pushConstantRanges;

	auto AddPushConstantRange = [](std::vector<VkPushConstantRange>& ranges, const KShaderInformation& information, VkShaderStageFlags stageFlag)
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
	};

	if (m_Shaders[VERTEX]) AddPushConstantRange(pushConstantRanges, m_Shaders[VERTEX]->GetInformation(), VK_SHADER_STAGE_VERTEX_BIT);
	if (m_Shaders[FRAGMENT]) AddPushConstantRange(pushConstantRanges, m_Shaders[FRAGMENT]->GetInformation(), VK_SHADER_STAGE_FRAGMENT_BIT);

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

bool KVulkanPipeline::CreateDestcriptionPool()
{
	// 更新描述集合	
	m_WriteDescriptorSet.clear();
	m_ImageWriteInfo.clear();
	m_BufferWriteInfo.clear();

	m_WriteDescriptorSet.reserve(m_Uniforms.size());
	m_ImageWriteInfo.resize(m_Samplers.size());
	m_BufferWriteInfo.resize(m_Uniforms.size());

	size_t bufferIdx = 0;
	size_t imageIdx = 0;

	for (auto& pair : m_Uniforms)
	{
		unsigned int location = pair.first;
		UniformBufferBindingInfo& info = pair.second;

		KVulkanUniformBuffer* uniformBuffer = static_cast<KVulkanUniformBuffer*>(info.buffer.get());
		ASSERT_RESULT(uniformBuffer != nullptr);

		VkDescriptorBufferInfo& bufferInfo = m_BufferWriteInfo[bufferIdx++];
		bufferInfo.buffer = uniformBuffer->GetVulkanHandle();
		bufferInfo.offset = 0;
		bufferInfo.range = uniformBuffer->GetBufferSize();

		VkWriteDescriptorSet uniformDescriptorWrite = {};

		uniformDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		// 写入的描述集合
		uniformDescriptorWrite.dstSet = VK_NULL_HANDLE;
		// 写入的位置 与DescriptorSetLayout里的VkDescriptorSetLayoutBinding位置对应
		uniformDescriptorWrite.dstBinding = location;
		// 写入索引与下面descriptorCount对应
		uniformDescriptorWrite.dstArrayElement = 0;

		uniformDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uniformDescriptorWrite.descriptorCount = 1;

		uniformDescriptorWrite.pBufferInfo = &bufferInfo;
		uniformDescriptorWrite.pImageInfo = nullptr; // Optional
		uniformDescriptorWrite.pTexelBufferView = nullptr; // Optional

		m_WriteDescriptorSet.push_back(uniformDescriptorWrite);
	}

	for (auto& pair : m_Samplers)
	{
		unsigned int location = pair.first;
		SamplerBindingInfo& info = pair.second;

		VkDescriptorImageInfo& imageInfo = m_ImageWriteInfo[imageIdx++];

		IKFrameBufferPtr frameBuffer = nullptr;
		if (info.texture)
		{
			frameBuffer = info.texture->GetFrameBuffer();
		}
		else if (info.target)
		{
			frameBuffer = info.target->GetFrameBuffer();
		}

		ASSERT_RESULT(frameBuffer);

		imageInfo.imageLayout = frameBuffer->IsStroageImage() ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageLayout = frameBuffer->IsDepthStencil() ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : imageInfo.imageLayout;
		imageInfo.imageView = ((KVulkanFrameBuffer*)frameBuffer.get())->GetImageView();
		imageInfo.sampler = ((KVulkanSampler*)info.sampler.get())->GetVkSampler();

		ASSERT_RESULT(imageInfo.imageView);
		ASSERT_RESULT(imageInfo.sampler);

		VkWriteDescriptorSet samplerDescriptorWrite = {};

		samplerDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		// 写入的描述集合
		samplerDescriptorWrite.dstSet = VK_NULL_HANDLE;
		// 写入的位置 与DescriptorSetLayout里的VkDescriptorSetLayoutBinding位置对应
		samplerDescriptorWrite.dstBinding = location;
		// 写入索引与下面descriptorCount对应
		samplerDescriptorWrite.dstArrayElement = 0;

		samplerDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		samplerDescriptorWrite.descriptorCount = 1;

		samplerDescriptorWrite.pBufferInfo = nullptr; // Optional
		samplerDescriptorWrite.pImageInfo = &imageInfo;
		samplerDescriptorWrite.pTexelBufferView = nullptr; // Optional

		m_WriteDescriptorSet.push_back(samplerDescriptorWrite);
	}

	m_Pool.Init(m_DescriptorSetLayout, m_DescriptorSetLayoutBinding, m_WriteDescriptorSet);

	return true;
}

VkDescriptorSet KVulkanPipeline::AllocDescriptorSet(const KDynamicConstantBufferUsage** ppConstantUsage, size_t dynamicBufferUsageCount,
	const KStorageBufferUsage** ppStorageUsage, size_t storageBufferUsageCount)
{
	return m_Pool.Alloc(KRenderGlobal::CurrentFrameIndex, KRenderGlobal::CurrentFrameNum, this, ppConstantUsage, dynamicBufferUsageCount, ppStorageUsage, storageBufferUsageCount);
}

bool KVulkanPipeline::Init()
{
	return true;
}

bool KVulkanPipeline::UnInit()
{
	DestroyDevice();
	ClearHandle();

	for (uint32_t i = 0; i < COUNT; ++i)
	{
		m_Shaders[i] = nullptr;
	}

	m_PushContant = { 0, 0 };

	m_Uniforms.clear();
	m_Samplers.clear();

	m_Pool.UnInit();

	return true;
}

bool KVulkanPipeline::Reload()
{
	DestroyDevice();
	ClearHandle();
	return true;
}

bool KVulkanPipeline::CheckDependencyResource()
{
	if (!m_Shaders[VERTEX] && !m_Shaders[MESH])
	{
		return false;
	}

	if (!m_Shaders[FRAGMENT])
	{
		return false;
	}

	for (uint32_t i = 0; i < COUNT; ++i)
	{
		if (m_Shaders[i] && m_Shaders[i]->GetResourceState() != RS_DEVICE_LOADED)
			return false;
	}

	for (auto& pair : m_Samplers)
	{
		unsigned int location = pair.first;
		SamplerBindingInfo& info = pair.second;

		if (info.texture)
		{
			if (info.texture->GetResourceState() != RS_DEVICE_LOADED)
			{
				return false;
			}
		}

		if (info.sampler->GetResourceState() != RS_DEVICE_LOADED)
		{
			return false;
		}
	}

	return true;
}

bool KVulkanPipeline::GetHandle(IKRenderPassPtr renderPass, IKPipelineHandlePtr& handle)
{
	if (!CheckDependencyResource())
	{
		return false;
	}

	if (m_DescriptorSetLayout == VK_NULL_HANDLE && m_PipelineLayout == VK_NULL_HANDLE)
	{
		CreateLayout();
		CreateDestcriptionPool();
	}

	if (renderPass)
	{
		auto it = m_HandleMap.find(renderPass.get());
		if (it != m_HandleMap.end())
		{
			handle = it->second;
			return true;
		}

		renderPass->RegisterInvalidCallback(&m_RenderPassInvalidCB);
		handle = IKPipelineHandlePtr(KNEW KVulkanPipelineHandle());
		handle->Init(this, renderPass.get());
		m_HandleMap[renderPass.get()] = handle;

		return true;
	}
	return false;
}

KVulkanPipelineHandle::KVulkanPipelineHandle()
	: m_GraphicsPipeline(VK_NULL_HANDLE)
{
}

KVulkanPipelineHandle::~KVulkanPipelineHandle()
{
	ASSERT_RESULT(m_GraphicsPipeline == VK_NULL_HANDLE);
}

bool KVulkanPipelineHandle::Init(IKPipeline* pipeline, IKRenderPass* renderPass)
{
	UnInit();

	ASSERT_RESULT(pipeline);
	ASSERT_RESULT(renderPass);

	ASSERT_RESULT(m_GraphicsPipeline == VK_NULL_HANDLE);

	KVulkanPipeline* vulkanPipeline = static_cast<KVulkanPipeline*>(pipeline);
	KVulkanRenderPass* vulkanRenderPass = static_cast<KVulkanRenderPass*>(renderPass);

	ASSERT_RESULT((vulkanPipeline->m_Shaders[KVulkanPipeline::VERTEX] || vulkanPipeline->m_Shaders[KVulkanPipeline::MESH]) && vulkanPipeline->m_Shaders[KVulkanPipeline::FRAGMENT]);
	ASSERT_RESULT(vulkanPipeline->m_PipelineLayout);

	VkSampleCountFlagBits msaaFlag = vulkanRenderPass->GetMSAAFlag();
	const KViewPortArea& area = vulkanRenderPass->GetViewPort();

	// 配置顶点输入信息
	VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

	vertexInputInfo.vertexBindingDescriptionCount = (uint32_t)vulkanPipeline->m_BindingDescriptions.size();
	vertexInputInfo.pVertexBindingDescriptions = vulkanPipeline->m_BindingDescriptions.data();
	vertexInputInfo.vertexAttributeDescriptionCount = (uint32_t)vulkanPipeline->m_AttributeDescriptions.size();
	vertexInputInfo.pVertexAttributeDescriptions = vulkanPipeline->m_AttributeDescriptions.data();

	// 配置顶点组装信息
	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = vulkanPipeline->m_PrimitiveTopology;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	// 配置视口裁剪
	VkViewport viewport = {};
	viewport.x = (float)area.x;
	viewport.y = (float)area.y;
	viewport.width = (float)area.width;
	viewport.height = (float)area.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor =
	{
		{ (int32_t)area.x, (int32_t)area.y },
		{ (uint32_t)area.width, (uint32_t)area.height }
	};

	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	// 配置光栅化信息
	VkPipelineRasterizationStateCreateInfo rasterizer = {};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = vulkanPipeline->m_PolygonMode;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = vulkanPipeline->m_CullMode;
	rasterizer.frontFace = vulkanPipeline->m_FrontFace;

	rasterizer.depthBiasEnable = vulkanPipeline->m_DepthBiasEnable;
	rasterizer.depthBiasConstantFactor = 0.0f; // Optional
	rasterizer.depthBiasClamp = 0.0f; // Optional
	rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

	// 配置深度缓冲信息
	VkPipelineDepthStencilStateCreateInfo depthStencil = {};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = vulkanPipeline->m_DepthTest;
	depthStencil.depthWriteEnable = vulkanPipeline->m_DepthWrite;
	depthStencil.depthCompareOp = vulkanPipeline->m_DepthCompareOp;

	depthStencil.depthBoundsTestEnable = VK_FALSE; // Optional
	depthStencil.minDepthBounds = 0.0f; // Optional
	depthStencil.maxDepthBounds = 1.0f; // Optional

	depthStencil.stencilTestEnable = vulkanPipeline->m_StencilEnable;

	VkStencilOpState stencilState = {};
	if (vulkanPipeline->m_StencilEnable)
	{
		stencilState.compareMask = 0xFF;
		stencilState.writeMask = 0xFF;

		stencilState.compareOp = vulkanPipeline->m_StencilCompareOp;
		stencilState.failOp = vulkanPipeline->m_StencilFailOp;
		stencilState.depthFailOp = vulkanPipeline->m_StencilDepthFailOp;
		stencilState.passOp = vulkanPipeline->m_StencilPassOp;
		stencilState.reference = vulkanPipeline->m_StencilRef;
	}
	depthStencil.front = stencilState;
	depthStencil.back = stencilState;

	// 配置多重采样信息
	VkPipelineMultisampleStateCreateInfo multisampling = {};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = msaaFlag;
	multisampling.minSampleShading = 1.0f; // Optional
	multisampling.pSampleMask = nullptr; // Optional
	multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
	multisampling.alphaToOneEnable = VK_FALSE; // Optional

	// 配置Alpha混合信息
	VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
	colorBlendAttachment.colorWriteMask = vulkanPipeline->m_ColorWriteMask;
	colorBlendAttachment.blendEnable = vulkanPipeline->m_BlendEnable;

	colorBlendAttachment.srcColorBlendFactor = vulkanPipeline->m_ColorSrcBlendFactor;
	colorBlendAttachment.dstColorBlendFactor = vulkanPipeline->m_ColorDstBlendFactor;
	colorBlendAttachment.colorBlendOp = vulkanPipeline->m_ColorBlendOp;

	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

	std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments;
	colorBlendAttachments.resize(vulkanRenderPass->GetColorAttachmentCount());
	for (size_t i = 0; i < colorBlendAttachments.size(); ++i)
	{
		colorBlendAttachments[i] = colorBlendAttachment;
	}

	VkPipelineColorBlendStateCreateInfo colorBlending = {};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional

	colorBlending.attachmentCount = vulkanRenderPass->GetColorAttachmentCount();
	colorBlending.pAttachments = colorBlendAttachments.data();

	colorBlending.blendConstants[0] = 0.0f; // Optional
	colorBlending.blendConstants[1] = 0.0f; // Optional
	colorBlending.blendConstants[2] = 0.0f; // Optional
	colorBlending.blendConstants[3] = 0.0f; // Optional

	// 设置动态状态
	std::vector<VkDynamicState> dynamicStates;

	dynamicStates.push_back(VK_DYNAMIC_STATE_VIEWPORT);
	dynamicStates.push_back(VK_DYNAMIC_STATE_SCISSOR);	
	dynamicStates.push_back(VK_DYNAMIC_STATE_DEPTH_BIAS);

	VkPipelineDynamicStateCreateInfo dynamicState = {};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = (uint32_t)dynamicStates.size();
	dynamicState.pDynamicStates = dynamicStates.data();

	std::vector<VkPipelineShaderStageCreateInfo> shaderStageInfo;
	KVulkanShader* vulkanShader = nullptr;

	for (uint32_t i = 0; i < KVulkanPipeline::COUNT; ++i)
	{
		if (vulkanPipeline->m_Shaders[i])
		{
			VkPipelineShaderStageCreateInfo shaderCreateInfo = {};
			vulkanShader = (KVulkanShader*)vulkanPipeline->m_Shaders[i].get();
			shaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			shaderCreateInfo.stage = SHADER_STAGE_FLAGS[i];
			shaderCreateInfo.module = vulkanShader->GetShaderModule();
			shaderCreateInfo.pSpecializationInfo = vulkanShader->GetSpecializationInfo();
			shaderCreateInfo.pName = "main";

			shaderStageInfo.push_back(shaderCreateInfo);
		}
	}

	// 创建管线
	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	// 指定管线所使用的Shader
	pipelineInfo.stageCount = (uint32_t)shaderStageInfo.size();
	pipelineInfo.pStages = shaderStageInfo.data();
	// 指定管线顶点输入信息
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	// 指定管线视口
	pipelineInfo.pViewportState = &viewportState;
	// 指定光栅化
	pipelineInfo.pRasterizationState = &rasterizer;
	// 指定深度缓冲
	pipelineInfo.pDepthStencilState = &depthStencil;
	// 指定多重采样方式
	pipelineInfo.pMultisampleState = &multisampling;
	// 指定混合模式
	pipelineInfo.pColorBlendState = &colorBlending;
	// 指定动态状态
	pipelineInfo.pDynamicState = &dynamicState;
	// 指定管线布局
	pipelineInfo.layout = vulkanPipeline->m_PipelineLayout;
	// 指定渲染通道
	pipelineInfo.renderPass = vulkanRenderPass->GetVkRenderPass();
	pipelineInfo.subpass = 0;

	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
	pipelineInfo.basePipelineIndex = -1; // Optional

	VK_ASSERT_RESULT(vkCreateGraphicsPipelines(KVulkanGlobal::device, KVulkanGlobal::pipelineCache, 1, &pipelineInfo, nullptr, &m_GraphicsPipeline));

	return true;
}

bool KVulkanPipelineHandle::UnInit()
{
	if (m_GraphicsPipeline != VK_NULL_HANDLE)
	{
		// Cannot call vkDestroyPipeline on VkPipeline  that is currently in use by a command buffer
		vkDeviceWaitIdle(KVulkanGlobal::device);
		vkDestroyPipeline(KVulkanGlobal::device, m_GraphicsPipeline, nullptr);
		m_GraphicsPipeline = VK_NULL_HANDLE;
	}
	return true;
}