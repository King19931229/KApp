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
	m_UniformBufferDescriptorCount(0),
	m_SamplerDescriptorCount(0),
	m_DescriptorSetLayout(VK_NULL_HANDLE),
	m_PipelineLayout(VK_NULL_HANDLE),
	/*
	m_DepthBiasConstantFactor(0),
	m_DepthBiasClamp(0),
	m_DepthBiasSlopeFactor(0),
	*/
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
	ASSERT_RESULT(m_VertexShader == nullptr);
	ASSERT_RESULT(m_FragmentShader == nullptr);
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

/*
bool KVulkanPipeline::SetDepthBias(float depthBiasConstantFactor, float depthBiasClamp, float depthBiasSlopeFactor)
{
	m_DepthBiasConstantFactor = depthBiasConstantFactor;
	m_DepthBiasClamp = depthBiasClamp;
	m_DepthBiasSlopeFactor = depthBiasSlopeFactor;
	return true;
}
*/

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
		m_VertexShader = shader;
		return true;
	case ST_FRAGMENT:
		m_FragmentShader = shader;
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

bool KVulkanPipeline::SetSamplerImpl(unsigned int location, IKTexturePtr texture, IKSamplerPtr sampler, bool dynamic)
{
	if (texture && sampler)
	{
		SamplerBindingInfo info;
		info.texture = texture;
		info.sampler = sampler;
		ASSERT_RESULT(BindSampler(location, info));
		return true;
	}
	return false;
}

bool KVulkanPipeline::SetSamplerAttachmentImpl(unsigned int location, IKRenderTargetPtr target, IKSamplerPtr sampler, bool dynamic)
{
	if (target && sampler)
	{
		SamplerBindingInfo info;
		info.frameBuffer = target->GetFrameBuffer();
		info.sampler = sampler;
		ASSERT_RESULT(BindSampler(location, info));
		return true;
	}
	return false;
}

bool KVulkanPipeline::SetSampler(unsigned int location, IKTexturePtr texture, IKSamplerPtr sampler)
{
	if(texture && sampler)
	{
		return SetSamplerImpl(location, texture, sampler, false);
	}
	return false;
}

bool KVulkanPipeline::SetSampler(unsigned int location, IKRenderTargetPtr target, IKSamplerPtr sampler)
{
	if(target && sampler)
	{
		return SetSamplerAttachmentImpl(location, target, sampler, false);
	}
	return false;
}

bool KVulkanPipeline::SetSamplerDynamic(unsigned int location, IKTexturePtr texture, IKSamplerPtr sampler)
{
	if (texture && sampler)
	{
		return SetSamplerImpl(location, texture, sampler, true);
	}
	return false;
}

bool KVulkanPipeline::SetSamplerDynamic(unsigned int location, IKRenderTargetPtr target, IKSamplerPtr sampler)
{
	if (target && sampler)
	{
		return SetSamplerAttachmentImpl(location, target, sampler, true);
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

	auto IsDuplicateLayoutBinding = [](const VkDescriptorSetLayoutBinding& lhs, const VkDescriptorSetLayoutBinding& rhs)->bool
	{
		if (lhs.binding != rhs.binding)
			return false;
		if (lhs.descriptorType != rhs.descriptorType)
			return false;
		if (lhs.descriptorCount != rhs.descriptorCount)
			return false;
		if (lhs.pImmutableSamplers != rhs.pImmutableSamplers)
			return false;
		return true;
	};

	// VertexShader Binding
	{
		const KShaderInformation& vertexInformation = m_VertexShader->GetInformation();

		for (const KShaderInformation::Constant& constant : vertexInformation.constants)
		{
			VkDescriptorSetLayoutBinding uboLayoutBinding = {};
			// 与Shader中绑定位置对应
			uboLayoutBinding.binding = constant.bindingIndex;
			uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			// 声明UBO Buffer数组长度 这里不使用数组
			uboLayoutBinding.descriptorCount = 1;
			// 声明哪个阶段Shader能够使用上此UBO
			uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
			uboLayoutBinding.pImmutableSamplers = nullptr; // Optional
			
			auto it = std::find_if(m_DescriptorSetLayoutBinding.begin(), m_DescriptorSetLayoutBinding.end(),
				[&uboLayoutBinding, &IsDuplicateLayoutBinding](const VkDescriptorSetLayoutBinding& lhs)->bool
			{
				return IsDuplicateLayoutBinding(lhs, uboLayoutBinding);
			});

			if (it == m_DescriptorSetLayoutBinding.end())
			{
				m_DescriptorSetLayoutBinding.push_back(uboLayoutBinding);
			}
			else
			{
				(*it).stageFlags |= VK_SHADER_STAGE_VERTEX_BIT;
			}
		}

		for (const KShaderInformation::Constant& constant : vertexInformation.dynamicConstants)
		{
			VkDescriptorSetLayoutBinding uboLayoutBinding = {};
			// 与Shader中绑定位置对应
			uboLayoutBinding.binding = constant.bindingIndex;
			uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
			// 声明UBO Buffer数组长度 这里不使用数组
			uboLayoutBinding.descriptorCount = 1;
			// 声明哪个阶段Shader能够使用上此UBO
			uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
			uboLayoutBinding.pImmutableSamplers = nullptr; // Optional

			auto it = std::find_if(m_DescriptorSetLayoutBinding.begin(), m_DescriptorSetLayoutBinding.end(),
				[&uboLayoutBinding, &IsDuplicateLayoutBinding](const VkDescriptorSetLayoutBinding& lhs)->bool
			{
				return IsDuplicateLayoutBinding(lhs, uboLayoutBinding);
			});

			if (it == m_DescriptorSetLayoutBinding.end())
			{
				m_DescriptorSetLayoutBinding.push_back(uboLayoutBinding);
			}
			else
			{
				(*it).stageFlags |= VK_SHADER_STAGE_VERTEX_BIT;
			}
		}

		for (const KShaderInformation::Texture& texture : vertexInformation.textures)
		{
			VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
			// 与Shader中绑定位置对应
			samplerLayoutBinding.binding = texture.bindingIndex;
			samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			// 这里不使用数组
			samplerLayoutBinding.descriptorCount = 1;
			// 声明哪个阶段Shader能够使用上此Sampler
			samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
			samplerLayoutBinding.pImmutableSamplers = nullptr; // Optional

			auto it = std::find_if(m_DescriptorSetLayoutBinding.begin(), m_DescriptorSetLayoutBinding.end(),
				[&samplerLayoutBinding, &IsDuplicateLayoutBinding](const VkDescriptorSetLayoutBinding& lhs)->bool
			{
				return IsDuplicateLayoutBinding(lhs, samplerLayoutBinding);
			});

			if (it == m_DescriptorSetLayoutBinding.end())
			{
				m_DescriptorSetLayoutBinding.push_back(samplerLayoutBinding);
			}
			else
			{
				(*it).stageFlags |= VK_SHADER_STAGE_VERTEX_BIT;
			}
		}
	}

	// FragmentShader Binding
	{
		const KShaderInformation& fragmentInformation = m_FragmentShader->GetInformation();

		for (const KShaderInformation::Constant& constant : fragmentInformation.constants)
		{
			VkDescriptorSetLayoutBinding uboLayoutBinding = {};
			// 与Shader中绑定位置对应
			uboLayoutBinding.binding = constant.bindingIndex;
			uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			// 声明UBO Buffer数组长度 这里不使用数组
			uboLayoutBinding.descriptorCount = 1;
			// 声明哪个阶段Shader能够使用上此UBO
			uboLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			uboLayoutBinding.pImmutableSamplers = nullptr; // Optional
			
			auto it = std::find_if(m_DescriptorSetLayoutBinding.begin(), m_DescriptorSetLayoutBinding.end(),
				[&uboLayoutBinding, &IsDuplicateLayoutBinding](const VkDescriptorSetLayoutBinding& lhs)->bool
			{
				return IsDuplicateLayoutBinding(lhs, uboLayoutBinding);
			});

			if (it == m_DescriptorSetLayoutBinding.end())
			{
				m_DescriptorSetLayoutBinding.push_back(uboLayoutBinding);
			}
			else
			{
				(*it).stageFlags |= VK_SHADER_STAGE_FRAGMENT_BIT;
			}
		}

		for (const KShaderInformation::Constant& constant : fragmentInformation.dynamicConstants)
		{
			VkDescriptorSetLayoutBinding uboLayoutBinding = {};
			// 与Shader中绑定位置对应
			uboLayoutBinding.binding = constant.bindingIndex;
			uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
			// 声明UBO Buffer数组长度 这里不使用数组
			uboLayoutBinding.descriptorCount = 1;
			// 声明哪个阶段Shader能够使用上此UBO
			uboLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			uboLayoutBinding.pImmutableSamplers = nullptr; // Optional

			auto it = std::find_if(m_DescriptorSetLayoutBinding.begin(), m_DescriptorSetLayoutBinding.end(),
				[&uboLayoutBinding, &IsDuplicateLayoutBinding](const VkDescriptorSetLayoutBinding& lhs)->bool
			{
				return IsDuplicateLayoutBinding(lhs, uboLayoutBinding);
			});

			if (it == m_DescriptorSetLayoutBinding.end())
			{
				m_DescriptorSetLayoutBinding.push_back(uboLayoutBinding);
			}
			else
			{
				(*it).stageFlags |= VK_SHADER_STAGE_FRAGMENT_BIT;
			}
		}

		for (const KShaderInformation::Texture& texture : fragmentInformation.textures)
		{
			VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
			// 与Shader中绑定位置对应
			samplerLayoutBinding.binding = texture.bindingIndex;
			samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			// 这里不使用数组
			samplerLayoutBinding.descriptorCount = 1;
			// 声明哪个阶段Shader能够使用上此Sampler
			samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			samplerLayoutBinding.pImmutableSamplers = nullptr; // Optional

			auto it = std::find_if(m_DescriptorSetLayoutBinding.begin(), m_DescriptorSetLayoutBinding.end(),
				[&samplerLayoutBinding, &IsDuplicateLayoutBinding](const VkDescriptorSetLayoutBinding& lhs)->bool
			{
				return IsDuplicateLayoutBinding(lhs, samplerLayoutBinding);
			});

			if (it == m_DescriptorSetLayoutBinding.end())
			{
				m_DescriptorSetLayoutBinding.push_back(samplerLayoutBinding);
			}
			else
			{
				(*it).stageFlags |= VK_SHADER_STAGE_FRAGMENT_BIT;
			}
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

	auto IsDuplicatePushConstantRange = [](const VkPushConstantRange& lhs, const VkPushConstantRange& rhs)->bool
	{
		if (lhs.offset != rhs.offset)
			return false;
		if (lhs.size != rhs.size)
			return false;
		return true;
	};

	// VertexShader Binding
	{
		const KShaderInformation& vertexInformation = m_VertexShader->GetInformation();

		for (const KShaderInformation::Constant& constant : vertexInformation.pushConstants)
		{
			VkPushConstantRange pushConstantRange = {};
			pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
			pushConstantRange.offset = 0;
			pushConstantRange.size = constant.size;

			auto it = std::find_if(pushConstantRanges.begin(), pushConstantRanges.end(),
				[&pushConstantRange, &IsDuplicatePushConstantRange](const VkPushConstantRange& lhs)->bool
			{
				return IsDuplicatePushConstantRange(lhs, pushConstantRange);
			});

			if (it == pushConstantRanges.end())
			{
				pushConstantRanges.push_back(pushConstantRange);
			}
			else
			{
				(*it).stageFlags |= VK_SHADER_STAGE_VERTEX_BIT;
			}
		}
	}

	// Fragment Binding
	{
		const KShaderInformation& fragmentInformation = m_FragmentShader->GetInformation();

		for (const KShaderInformation::Constant& constant : fragmentInformation.pushConstants)
		{
			VkPushConstantRange pushConstantRange = {};
			pushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			pushConstantRange.offset = 0;
			pushConstantRange.size = constant.size;

			auto it = std::find_if(pushConstantRanges.begin(), pushConstantRanges.end(),
				[&pushConstantRange, &IsDuplicatePushConstantRange](const VkPushConstantRange& lhs)->bool
			{
				return IsDuplicatePushConstantRange(lhs, pushConstantRange);
			});

			if (it == pushConstantRanges.end())
			{
				pushConstantRanges.push_back(pushConstantRange);
			}
			else
			{
				(*it).stageFlags |= VK_SHADER_STAGE_FRAGMENT_BIT;
			}
		}
	}

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

		if (!info.frameBuffer && info.texture)
		{
			info.frameBuffer = info.texture->GetFrameBuffer();
		}

		imageInfo.imageLayout = info.frameBuffer->IsDepthStencil() ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;		
		imageInfo.imageView = ((KVulkanFrameBuffer*)info.frameBuffer.get())->GetImageView();
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

VkDescriptorSet KVulkanPipeline::AllocDescriptorSet(const KDynamicConstantBufferUsage** ppBufferUsage, size_t dynamicBufferUsageCount, const KDynamicTextureUsage* pTextureUsage, size_t dynamicTextureUsageCount)
{
	return m_Pool.Alloc(KRenderGlobal::CurrentFrameIndex, KRenderGlobal::CurrentFrameNum, this, ppBufferUsage, dynamicBufferUsageCount, pTextureUsage, dynamicTextureUsageCount);
}

bool KVulkanPipeline::Init()
{
	return true;
}

bool KVulkanPipeline::UnInit()
{
	DestroyDevice();
	ClearHandle();

	m_VertexShader = nullptr;
	m_FragmentShader = nullptr;

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
	if (!m_VertexShader || m_VertexShader->GetResourceState() != RS_DEVICE_LOADED)
	{
		return false;
	}

	if (!m_FragmentShader || m_FragmentShader->GetResourceState() != RS_DEVICE_LOADED)
	{
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

	ASSERT_RESULT(vulkanPipeline->m_VertexShader && vulkanPipeline->m_FragmentShader);
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

	VkPipelineColorBlendStateCreateInfo colorBlending = {};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional

	colorBlending.attachmentCount = vulkanRenderPass->GetColorAttachmentCount();
	colorBlending.pAttachments = &colorBlendAttachment;

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

	// vs
	VkPipelineShaderStageCreateInfo vsShaderCreateInfo = {};
	vulkanShader = (KVulkanShader*)vulkanPipeline->m_VertexShader.get();
	vsShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vsShaderCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vsShaderCreateInfo.module = vulkanShader->GetShaderModule();
	vsShaderCreateInfo.pSpecializationInfo = vulkanShader->GetSpecializationInfo();
	vsShaderCreateInfo.pName = "main";

	shaderStageInfo.push_back(vsShaderCreateInfo);

	// fs
	VkPipelineShaderStageCreateInfo fsShaderCreateInfo = {};
	vulkanShader = (KVulkanShader*)vulkanPipeline->m_FragmentShader.get();
	fsShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fsShaderCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fsShaderCreateInfo.module = vulkanShader->GetShaderModule();
	fsShaderCreateInfo.pSpecializationInfo = vulkanShader->GetSpecializationInfo();
	fsShaderCreateInfo.pName = "main";

	shaderStageInfo.push_back(fsShaderCreateInfo);

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
		vkDestroyPipeline(KVulkanGlobal::device, m_GraphicsPipeline, nullptr);
		m_GraphicsPipeline = VK_NULL_HANDLE;
	}
	return true;
}