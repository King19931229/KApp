#include "KVulkanPipeline.h"
#include "KVulkanBuffer.h"
#include "KVulkanTexture.h"
#include "KVulkanSampler.h"
#include "KVulkanHelper.h"
#include "KVulkanProgram.h"
#include "KVulkanRenderTarget.h"
#include "KVulkanGlobal.h"

#include "Internal/KRenderGlobal.h"

KVulkanPipeline::KVulkanPipeline() :
	m_PrimitiveTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST),
	m_ColorSrcBlendFactor(VK_BLEND_FACTOR_SRC_ALPHA),
	m_ColorDstBlendFactor(VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA),
	m_ColorBlendOp(VK_BLEND_OP_ADD),
	m_BlendEnable(VK_FALSE),
	m_PolygonMode(VK_POLYGON_MODE_FILL),
	m_CullMode(VK_CULL_MODE_BACK_BIT),
	m_FrontFace(VK_FRONT_FACE_COUNTER_CLOCKWISE),
	m_DepthWrite(VK_TRUE),
	m_DepthTest(VK_TRUE),
	m_DepthOp(VK_COMPARE_OP_LESS_OR_EQUAL),
	m_DescriptorSetLayout(VK_NULL_HANDLE),
	m_DescriptorPool(VK_NULL_HANDLE),
	m_DescriptorSet(VK_NULL_HANDLE),
	m_PipelineLayout(VK_NULL_HANDLE),
	m_Program(IKProgramPtr(new KVulkanProgram()))
{

}

KVulkanPipeline::~KVulkanPipeline()
{
	ASSERT_RESULT(m_DescriptorSetLayout == VK_NULL_HANDLE);
	ASSERT_RESULT(m_DescriptorPool == VK_NULL_HANDLE);
	ASSERT_RESULT(m_DescriptorSet == VK_NULL_HANDLE);
	ASSERT_RESULT(m_PipelineLayout == VK_NULL_HANDLE);
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
	ASSERT_RESULT(KVulkanHelper::CompareFuncToVkCompareOp(func, m_DepthOp));
	m_DepthWrite = (VkBool32)depthWrtie;
	m_DepthTest = (VkBool32)depthTest;
	return true;
}

bool KVulkanPipeline::SetShader(ShaderTypeFlag shaderType, IKShaderPtr shader)
{
	ASSERT_RESULT(m_Program->AttachShader(shaderType, shader));
	return true;
}

bool KVulkanPipeline::SetConstantBuffer(unsigned int location, ShaderTypes shaderTypes, IKUniformBufferPtr buffer)
{
	if(buffer)
	{
		ASSERT_RESULT(m_Samplers.find(location) == m_Samplers.end() && "The location you try to bind is conflited with sampler");

		UniformBufferBindingInfo info = { shaderTypes, buffer };
		auto& it = m_Uniforms.find(location);
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

bool KVulkanPipeline::SetSampler(unsigned int location, const ImageView& imageView, IKSamplerPtr sampler)
{
	if(imageView.imageViewHandle != nullptr && sampler)
	{
		ASSERT_RESULT(m_Uniforms.find(location) == m_Uniforms.end() && "The location you try to bind is conflited with ubo");

		SamplerBindingInfo info =
		{
			(VkImageView)imageView.imageViewHandle,
			((KVulkanSampler*)sampler.get())->GetVkSampler()
		};
		auto& it = m_Samplers.find(location);
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
	return false;
}

bool KVulkanPipeline::PushConstantBlock(ShaderTypes shaderTypes, uint32_t size, uint32_t& offset)
{
	offset = 0;
	for(auto& info : m_PushContants)
	{
		offset += info.size;
	}

	PushConstantBindingInfo info = { shaderTypes, size, offset };
	m_PushContants.push_back(info);
	return true;
}

bool KVulkanPipeline::CreateLayout()
{
	ASSERT_RESULT(m_DescriptorSetLayout == VK_NULL_HANDLE);
	ASSERT_RESULT(m_PipelineLayout == VK_NULL_HANDLE);
	
	/*
	DescriptorSetLayout ��������UBO Sampler�󶨵�λ��
	ʵ��UBO Sampler �����������������ָ��
	*/
	std::vector<VkDescriptorSetLayoutBinding> layoutBindings;

	for(auto& pair : m_Uniforms)
	{
		unsigned int location = pair.first;
		UniformBufferBindingInfo& info = pair.second;

		VkFlags stageFlags = 0;
		ASSERT_RESULT(KVulkanHelper::ShaderTypesToVkShaderStageFlag(info.shaderTypes, stageFlags));

		VkDescriptorSetLayoutBinding uboLayoutBinding = {};
		// ��Shader�а�λ�ö�Ӧ
		uboLayoutBinding.binding = location;
		uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		// ����UBO Buffer���鳤�� ���ﲻʹ������
		uboLayoutBinding.descriptorCount = 1;
		// �����ĸ��׶�Shader�ܹ�ʹ���ϴ�UBO
		uboLayoutBinding.stageFlags = stageFlags; // VK_SHADER_STAGE_ALL_GRAPHICS
		uboLayoutBinding.pImmutableSamplers = nullptr; // Optional

		layoutBindings.push_back(uboLayoutBinding);
	}

	for(auto& pair : m_Samplers)
	{
		unsigned int location = pair.first;
		SamplerBindingInfo& info = pair.second;

		VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
		// ��Shader�а�λ�ö�Ӧ
		samplerLayoutBinding.binding = location;
		samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		// ���ﲻʹ������
		samplerLayoutBinding.descriptorCount = 1;
		// �����ĸ��׶�Shader�ܹ�ʹ���ϴ�Sampler
		samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		samplerLayoutBinding.pImmutableSamplers = nullptr; // Optional

		layoutBindings.push_back(samplerLayoutBinding);
	}

	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(layoutBindings.size());
	layoutInfo.pBindings = layoutBindings.data();

	VK_ASSERT_RESULT(vkCreateDescriptorSetLayout(KVulkanGlobal::device, &layoutInfo, nullptr, &m_DescriptorSetLayout));

	/*
	����PushConstant
	*/
	size_t pushConstantOffset = 0;
	std::vector<VkPushConstantRange> pushConstantRanges;
	for(auto& info : m_PushContants)
	{
		VkFlags stageFlags = 0;
		ASSERT_RESULT(KVulkanHelper::ShaderTypesToVkShaderStageFlag(info.shaderTypes, stageFlags));

		VkPushConstantRange pushConstantRange = {};
		pushConstantRange.stageFlags = stageFlags;
		pushConstantRange.offset = (uint32_t)pushConstantOffset;
		pushConstantRange.size = (uint32_t)info.size;

		pushConstantRanges.push_back(pushConstantRange);

		pushConstantOffset += pushConstantRange.size;
	}

	// �������߲���
	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	// ָ���ù��ߵ���������
	pipelineLayoutInfo.pSetLayouts = &m_DescriptorSetLayout;
	// ָ��PushConstant
	pipelineLayoutInfo.pushConstantRangeCount = static_cast<uint32_t>(pushConstantRanges.size());
	pipelineLayoutInfo.pPushConstantRanges = pushConstantRanges.size() > 0 ? pushConstantRanges.data() : nullptr;

	VK_ASSERT_RESULT(vkCreatePipelineLayout(KVulkanGlobal::device, &pipelineLayoutInfo, nullptr, &m_PipelineLayout));

	return true;
}

bool KVulkanPipeline::CreateDestcription()
{
	ASSERT_RESULT(m_DescriptorSetLayout != VK_NULL_HANDLE);
	ASSERT_RESULT(m_DescriptorPool == VK_NULL_HANDLE);
	ASSERT_RESULT(m_DescriptorSet == VK_NULL_HANDLE);

	VkDescriptorPoolSize uniformPoolSize = {};
	uniformPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	// �������ش�����type(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)���������������ֵ
	uniformPoolSize.descriptorCount = std::max(1U, static_cast<uint32_t>(m_Uniforms.size()));

	VkDescriptorPoolSize samplerPoolSize = {};
	samplerPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	// �������ش�����type(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)���������������ֵ
	samplerPoolSize.descriptorCount = std::max(1U, static_cast<uint32_t>(m_Samplers.size()));

	VkDescriptorPoolSize poolSizes[] = {uniformPoolSize, samplerPoolSize};

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = ARRAY_SIZE(poolSizes);
	poolInfo.pPoolSizes = poolSizes;

	// �����������ظ���type���������������ܺ�
	// �൱������pPoolSizes��descriptorCount�ܺ�
	poolInfo.maxSets = poolInfo.poolSizeCount;

	VK_ASSERT_RESULT(vkCreateDescriptorPool(KVulkanGlobal::device, &poolInfo, nullptr, &m_DescriptorPool));

	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	// ָ�����������ϴ��ĸ������ش���
	allocInfo.descriptorPool = m_DescriptorPool;
	// ����������������
	allocInfo.descriptorSetCount = 1;
	// ָ��ÿ���������������϶�Ӧ����������
	allocInfo.pSetLayouts = &m_DescriptorSetLayout;

	VK_ASSERT_RESULT(vkAllocateDescriptorSets(KVulkanGlobal::device, &allocInfo, &m_DescriptorSet));

	// ������������
	std::vector<VkWriteDescriptorSet> writeDescriptorSet;
	std::vector<VkDescriptorBufferInfo> descBufferInfo;
	std::vector<VkDescriptorImageInfo> descImageInfo;

	writeDescriptorSet.reserve(m_Uniforms.size());
	descBufferInfo.reserve(m_Uniforms.size());
	descImageInfo.reserve(m_Samplers.size());

	for(auto& pair : m_Uniforms)
	{
		unsigned int location = pair.first;
		UniformBufferBindingInfo& info = pair.second;

		KVulkanUniformBuffer* uniformBuffer = static_cast<KVulkanUniformBuffer*>(info.buffer.get());
		ASSERT_RESULT(uniformBuffer != nullptr);

		VkDescriptorBufferInfo bufferInfo = {};
		bufferInfo.buffer = uniformBuffer->GetVulkanHandle();
		bufferInfo.offset = 0;
		bufferInfo.range = uniformBuffer->GetBufferSize();

		descBufferInfo.push_back(bufferInfo);

		VkWriteDescriptorSet uniformDescriptorWrite = {};

		uniformDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		// д�����������
		uniformDescriptorWrite.dstSet = m_DescriptorSet;
		// д���λ�� ��DescriptorSetLayout���VkDescriptorSetLayoutBindingλ�ö�Ӧ
		uniformDescriptorWrite.dstBinding = location;
		// д������������descriptorCount��Ӧ
		uniformDescriptorWrite.dstArrayElement = 0;

		uniformDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uniformDescriptorWrite.descriptorCount = 1;

		uniformDescriptorWrite.pBufferInfo = &descBufferInfo.back();
		uniformDescriptorWrite.pImageInfo = nullptr; // Optional
		uniformDescriptorWrite.pTexelBufferView = nullptr; // Optional

		writeDescriptorSet.push_back(uniformDescriptorWrite);
	}

	for(auto& pair : m_Samplers)
	{
		unsigned int location = pair.first;
		SamplerBindingInfo& info = pair.second;

		VkDescriptorImageInfo imageInfo = {};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = info.vkImageView;
		imageInfo.sampler = info.vkSampler;

		descImageInfo.push_back(imageInfo);

		VkWriteDescriptorSet samplerDescriptorWrite = {};

		samplerDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET ;
		// д�����������
		samplerDescriptorWrite.dstSet = m_DescriptorSet;
		// д���λ�� ��DescriptorSetLayout���VkDescriptorSetLayoutBindingλ�ö�Ӧ
		samplerDescriptorWrite.dstBinding = location;
		// д������������descriptorCount��Ӧ
		samplerDescriptorWrite.dstArrayElement = 0;

		samplerDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		samplerDescriptorWrite.descriptorCount = 1;

		samplerDescriptorWrite.pBufferInfo = nullptr; // Optional
		samplerDescriptorWrite.pImageInfo = &descImageInfo.back();
		samplerDescriptorWrite.pTexelBufferView = nullptr; // Optional

		writeDescriptorSet.push_back(samplerDescriptorWrite);
	}

	vkUpdateDescriptorSets(KVulkanGlobal::device, static_cast<uint32_t>(writeDescriptorSet.size()), writeDescriptorSet.data(), 0, nullptr);

	return true;
}

bool KVulkanPipeline::Init()
{	
	ASSERT_RESULT(m_Program->Init());
	ASSERT_RESULT(CreateLayout());
	ASSERT_RESULT(CreateDestcription());
	return true;
}

bool KVulkanPipeline::UnInit()
{
	if(m_DescriptorSet)
	{
		m_DescriptorSet = VK_NULL_HANDLE;
	}
	if(m_DescriptorPool)
	{
		vkDestroyDescriptorPool(KVulkanGlobal::device, m_DescriptorPool, nullptr);
		m_DescriptorPool = VK_NULL_HANDLE;
	}
	if(m_DescriptorSetLayout)
	{
		vkDestroyDescriptorSetLayout(KVulkanGlobal::device, m_DescriptorSetLayout, nullptr);
		m_DescriptorSetLayout = VK_NULL_HANDLE;
	}

	if(m_PipelineLayout)
	{
		vkDestroyPipelineLayout(KVulkanGlobal::device, m_PipelineLayout, nullptr);
		m_PipelineLayout = VK_NULL_HANDLE;
	}

	KRenderGlobal::PipelineManager.InvaildateHandleByPipeline(this);

	m_Uniforms.clear();
	m_PushContants.clear();
	m_Samplers.clear();

	ASSERT_RESULT(m_Program->UnInit());

	return true;
}

KVulkanPipelineHandle::KVulkanPipelineHandle()
	: m_Pipeline(nullptr),
	m_Target(nullptr),
	m_GraphicsPipeline(VK_NULL_HANDLE)
{
}

KVulkanPipelineHandle::~KVulkanPipelineHandle()
{
	ASSERT_RESULT(m_Pipeline == nullptr);
	ASSERT_RESULT(m_Target == nullptr);
	ASSERT_RESULT(m_GraphicsPipeline == VK_NULL_HANDLE);
}

bool KVulkanPipelineHandle::Init(IKPipeline* pipeline, IKRenderTarget* target)
{
	ASSERT_RESULT(m_Pipeline == nullptr);
	ASSERT_RESULT(m_Target == nullptr);
	ASSERT_RESULT(m_GraphicsPipeline == VK_NULL_HANDLE);

	m_Pipeline = static_cast<KVulkanPipeline*>(pipeline);
	m_Target = static_cast<KVulkanRenderTarget*>(target);
	
	KVulkanProgram* program = static_cast<KVulkanProgram*>(m_Pipeline->m_Program.get());
	ASSERT_RESULT(program != nullptr);

	ASSERT_RESULT(m_Target != nullptr);
	VkSampleCountFlagBits msaaFlag = m_Target->GetMsaaFlag();
	VkExtent2D extend = m_Target->GetExtend();

	// ���ö���������Ϣ
	VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;	

	vertexInputInfo.vertexBindingDescriptionCount = (uint32_t)m_Pipeline->m_BindingDescriptions.size();
	vertexInputInfo.pVertexBindingDescriptions = m_Pipeline->m_BindingDescriptions.data();
	vertexInputInfo.vertexAttributeDescriptionCount = (uint32_t)m_Pipeline->m_AttributeDescriptions.size();
	vertexInputInfo.pVertexAttributeDescriptions = m_Pipeline->m_AttributeDescriptions.data();

	// ���ö�����װ��Ϣ
	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = m_Pipeline->m_PrimitiveTopology;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	// �����ӿڲü�
	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float) extend.width;
	viewport.height = (float) extend.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = 
	{
		{0, 0},
		extend
	};

	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	// ���ù�դ����Ϣ
	VkPipelineRasterizationStateCreateInfo rasterizer = {};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = m_Pipeline->m_PolygonMode;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = m_Pipeline->m_CullMode;
	rasterizer.frontFace = m_Pipeline->m_FrontFace;

	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f; // Optional
	rasterizer.depthBiasClamp = 0.0f; // Optional
	rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

	// ������Ȼ�����Ϣ
	VkPipelineDepthStencilStateCreateInfo depthStencil = {};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = m_Pipeline->m_DepthTest;
	depthStencil.depthWriteEnable = m_Pipeline->m_DepthWrite;
	depthStencil.depthCompareOp = m_Pipeline->m_DepthOp;

	depthStencil.depthBoundsTestEnable = VK_FALSE; // Optional
	depthStencil.minDepthBounds = 0.0f; // Optional
	depthStencil.maxDepthBounds = 1.0f; // Optional
	// TODO
	depthStencil.stencilTestEnable = VK_FALSE;

	VkStencilOpState empty = {};
	depthStencil.front = empty; // Optional
	depthStencil.back = empty; // Optional

	// ���ö��ز�����Ϣ
	VkPipelineMultisampleStateCreateInfo multisampling = {};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = msaaFlag;
	multisampling.minSampleShading = 1.0f; // Optional
	multisampling.pSampleMask = nullptr; // Optional
	multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
	multisampling.alphaToOneEnable = VK_FALSE; // Optional

	// ����Alpha�����Ϣ
	VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = m_Pipeline->m_BlendEnable;

	colorBlendAttachment.srcColorBlendFactor = m_Pipeline->m_ColorSrcBlendFactor;
	colorBlendAttachment.dstColorBlendFactor = m_Pipeline->m_ColorDstBlendFactor;
	colorBlendAttachment.colorBlendOp = m_Pipeline->m_ColorBlendOp;

	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

	VkPipelineColorBlendStateCreateInfo colorBlending = {};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f; // Optional
	colorBlending.blendConstants[1] = 0.0f; // Optional
	colorBlending.blendConstants[2] = 0.0f; // Optional
	colorBlending.blendConstants[3] = 0.0f; // Optional

	// ���ö�̬״̬
	VkDynamicState dynamicStates[] = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR,
		VK_DYNAMIC_STATE_LINE_WIDTH
	};

	VkPipelineDynamicStateCreateInfo dynamicState = {};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = ARRAY_SIZE(dynamicStates);
	dynamicState.pDynamicStates = dynamicStates;

	// ����Shader��Ϣ
	const std::vector<VkPipelineShaderStageCreateInfo> shaderStageInfo = program->GetShaderStageInfo();

	// ��������
	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	// ָ��������ʹ�õ�Shader
	pipelineInfo.stageCount = (uint32_t)shaderStageInfo.size();
	pipelineInfo.pStages = shaderStageInfo.data();
	// ָ�����߶���������Ϣ
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	// ָ�������ӿ�
	pipelineInfo.pViewportState = &viewportState;
	// ָ����դ��
	pipelineInfo.pRasterizationState = &rasterizer;
	// ָ����Ȼ���
	pipelineInfo.pDepthStencilState = &depthStencil;
	// ָ�����ز�����ʽ
	pipelineInfo.pMultisampleState = &multisampling;
	// ָ�����ģʽ
	pipelineInfo.pColorBlendState = &colorBlending;
	// ָ����̬״̬
	pipelineInfo.pDynamicState = &dynamicState;
	// ָ�����߲���
	pipelineInfo.layout = m_Pipeline->m_PipelineLayout;
	// ָ����Ⱦͨ��
	pipelineInfo.renderPass = m_Target->GetRenderPass();
	pipelineInfo.subpass = 0;

	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
	pipelineInfo.basePipelineIndex = -1; // Optional

	VK_ASSERT_RESULT(vkCreateGraphicsPipelines(KVulkanGlobal::device, KVulkanGlobal::pipelineCache, 1, &pipelineInfo, nullptr, &m_GraphicsPipeline));
	
	return true;
}

bool KVulkanPipelineHandle::UnInit()
{
	if(m_GraphicsPipeline)
	{
		vkDestroyPipeline(KVulkanGlobal::device, m_GraphicsPipeline, nullptr);
		m_GraphicsPipeline = VK_NULL_HANDLE;
	}
	m_Target = nullptr;
	m_Pipeline = nullptr;
	return true;
}