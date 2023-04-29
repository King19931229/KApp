#include "KVulkanPipeline.h"
#include "KVulkanPipelineLayout.h"
#include "KVulkanBuffer.h"
#include "KVulkanTexture.h"
#include "KVulkanSampler.h"
#include "KVulkanHelper.h"
#include "KVulkanShader.h"
#include "KVulkanRenderTarget.h"
#include "KVulkanRenderPass.h"
#include "KVulkanFrameBuffer.h"
#include "KVulkanGlobal.h"
#include "Internal/KRenderGlobal.h"
#include "KBase/Publish/KSectionEnterAssertGuard.h"

KVulkanPipeline::KVulkanPipeline()
	: m_DescriptorSetLayout(VK_NULL_HANDLE)
	, m_PipelineLayout(VK_NULL_HANDLE)
{
	m_RenderPassInvalidCB = [this](IKRenderPass* renderPass)
	{
		InvaildHandle(renderPass);
	};
}

KVulkanPipeline::~KVulkanPipeline()
{
}

bool KVulkanPipeline::InvaildHandle(IKRenderPass* renderPass)
{
	if (renderPass)
	{
		auto it = m_HandleMap.find(renderPass);
		if (it != m_HandleMap.end())
		{
			KRenderGlobal::PipelineManager.InvalidateHandle(it->second.hash);
			m_HandleMap.erase(it);
		}
	}
	return true;
}

bool KVulkanPipeline::DestroyDevice()
{
	m_Layout.Release();

	m_DescriptorSetLayout = VK_NULL_HANDEL;
	m_PipelineLayout = VK_NULL_HANDEL;
	m_DescriptorSetLayoutBinding.clear();

	for (auto& pair : m_HandleMap)
	{
		IKRenderPass* pass = pair.first;
		PipelineHandle& handle = pair.second;
		KRenderGlobal::PipelineManager.InvalidateHandle(handle.hash);
		pass->UnRegisterInvalidCallback(&m_RenderPassInvalidCB);
	}
	m_HandleMap.clear();

	m_WriteDescriptorSet.clear();
	m_BufferWriteInfo.clear();

	for (size_t i = 0; i < m_Pools.size(); ++i)
	{
		if (m_PoolInitializeds[i])
		{
			m_Pools[i].UnInit();
			m_PoolInitializeds[i] = false;
#ifdef _DEBUG
			m_PoolInitialing[i] = false;
#endif
		}
	}

	return true;
}

bool KVulkanPipeline::CreateLayout()
{
	if (KRenderGlobal::PipelineManager.AcquireLayout(m_Binding, m_Layout))
	{
		KVulkanPipelineLayout* vulkanPipelineLayout = (KVulkanPipelineLayout*)m_Layout.Get().get();
		m_DescriptorSetLayout = vulkanPipelineLayout->GetDescriptorSetLayout();
		m_PipelineLayout = vulkanPipelineLayout->GetPipelineLayout();
		m_DescriptorSetLayoutBinding = vulkanPipelineLayout->GetDescriptorSetLayoutBinding();
		CreateDestcriptionWrite();
		return true;
	}
	else
	{
		return false;
	}
}

bool KVulkanPipeline::CreateDestcriptionWrite()
{
	m_WriteDescriptorSet.clear();
	m_WriteDescriptorSet.reserve(m_Uniforms.size());

	m_BufferWriteInfo.clear();
	m_BufferWriteInfo.resize(m_Uniforms.size());

	size_t idx = 0;
	for (auto& pair : m_Uniforms)
	{
		uint32_t binding = pair.first;
		KVulkanUniformBuffer* vulkanUniformBuffer = (KVulkanUniformBuffer*)pair.second.buffer.get();

		VkDescriptorBufferInfo& bufferWrite = m_BufferWriteInfo[idx];
		bufferWrite.buffer = vulkanUniformBuffer->GetVulkanHandle();
		bufferWrite.offset = 0;
		bufferWrite.range = (VkDeviceSize)vulkanUniformBuffer->GetBufferSize();

		VkWriteDescriptorSet write = {};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.pNext = nullptr;
		write.dstSet = VK_NULL_HANDLE;
		write.dstBinding = binding;
		write.dstArrayElement = 0;
		write.descriptorCount = 1;
		write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

		write.pBufferInfo = &bufferWrite;
		write.pImageInfo = nullptr;
		write.pTexelBufferView = nullptr;

		m_WriteDescriptorSet.push_back(write);
		++idx;
	}		

	return true;
}

bool KVulkanPipeline::CreateDestcriptionPool(uint32_t threadIndex)
{
	if (m_Layout)
	{
#ifdef _DEBUG
		KSectionEnterAssertGuard gurad(m_PoolInitialing[threadIndex]);
#endif
		if (!m_PoolInitializeds[threadIndex])
		{
			KVulkanDescriptorPool pool;
			pool.Init(m_DescriptorSetLayout, m_DescriptorSetLayoutBinding, m_WriteDescriptorSet);
			if (!m_Name.empty())
			{
				pool.SetDebugName(m_Name + "_Thread" + std::to_string(threadIndex));
			}
			m_Pools[threadIndex].UnInit();
			m_Pools[threadIndex] = std::move(pool);
			m_PoolInitializeds[threadIndex] = true;
		}
		return true;
	}
	else
	{
		return false;
	}
}

VkDescriptorSet KVulkanPipeline::AllocDescriptorSet(uint32_t threadIndex,
	const KDynamicConstantBufferUsage** ppConstantUsage, size_t dynamicBufferUsageCount,
	const KStorageBufferUsage** ppStorageUsage, size_t storageBufferUsageCount)
{
	threadIndex = std::min(threadIndex, (uint32_t)(m_Pools.size() - 1));
	CreateDestcriptionPool(threadIndex);
	return m_Pools[threadIndex].Alloc(KRenderGlobal::CurrentInFlightFrameIndex, KRenderGlobal::CurrentFrameNum, this, ppConstantUsage, dynamicBufferUsageCount, ppStorageUsage, storageBufferUsageCount);
}

bool KVulkanPipeline::Init()
{
	KPipelineBase::Init();
	return true;
}

bool KVulkanPipeline::UnInit()
{
	KPipelineBase::UnInit();
	DestroyDevice();
	return true;
}

bool KVulkanPipeline::Reload()
{
	KPipelineBase::Reload();
	DestroyDevice();
	return true;
}

bool KVulkanPipeline::GetHandle(IKRenderPassPtr renderPass, IKPipelineHandlePtr& handle)
{
	if (!m_Layout)
	{
		CreateLayout();
	}

	if (renderPass)
	{
		auto it = m_HandleMap.find(renderPass.get());
		if (it != m_HandleMap.end())
		{
			handle = *it->second.handle;
			return true;
		}

		PipelineHandle newHandle;
		if (KRenderGlobal::PipelineManager.AcquireHandle(m_Layout.Get().get(), renderPass.get(), m_State, m_Binding, newHandle.handle, newHandle.hash))
		{
			m_HandleMap[renderPass.get()] = newHandle;
			handle = newHandle.handle.Get();
			handle->SetDebugName(m_Name.c_str());
			renderPass->RegisterInvalidCallback(&m_RenderPassInvalidCB);
			return true;
		}
	}
	return false;
}

KVulkanPipelineHandle::KVulkanPipelineHandle()
	: m_GraphicsPipeline(VK_NULL_HANDLE)
	, m_PrimitiveTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
	, m_ColorWriteMask(VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT)
	, m_ColorSrcBlendFactor(VK_BLEND_FACTOR_SRC_ALPHA)
	, m_ColorDstBlendFactor(VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA)
	, m_ColorBlendOp(VK_BLEND_OP_ADD)
	, m_BlendEnable(VK_FALSE)
	, m_PolygonMode(VK_POLYGON_MODE_FILL)
	, m_CullMode(VK_CULL_MODE_BACK_BIT)
	, m_FrontFace(VK_FRONT_FACE_COUNTER_CLOCKWISE)
	, m_DepthWrite(VK_TRUE)
	, m_DepthTest(VK_TRUE)
	, m_DepthCompareOp(VK_COMPARE_OP_LESS_OR_EQUAL)
	, m_DepthBiasEnable(VK_FALSE)
	, m_StencilFailOp(VK_STENCIL_OP_KEEP)
	, m_StencilDepthFailOp(VK_STENCIL_OP_KEEP)
	, m_StencilPassOp(VK_STENCIL_OP_KEEP)
	, m_StencilCompareOp(VK_COMPARE_OP_ALWAYS)
	, m_StencilRef(0)
	, m_StencilEnable(VK_FALSE)
{
}

KVulkanPipelineHandle::~KVulkanPipelineHandle()
{
	ASSERT_RESULT(m_GraphicsPipeline == VK_NULL_HANDLE);
}

bool KVulkanPipelineHandle::InitializeVulkanTerminology(const KPipelineState& state, const KPipelineBinding& binding)
{
	ASSERT_RESULT(KVulkanHelper::TopologyToVkPrimitiveTopology(state.topology, m_PrimitiveTopology));

	KVulkanHelper::VulkanBindingDetailList bindingDetails;
	ASSERT_RESULT(KVulkanHelper::PopulateInputBindingDescription(binding.formats.data(), binding.formats.size(), bindingDetails));

	m_BindingDescriptions.clear();
	m_AttributeDescriptions.clear();

	for (KVulkanHelper::VulkanBindingDetail& detail : bindingDetails)
	{
		m_BindingDescriptions.push_back(detail.bindingDescription);
		m_AttributeDescriptions.insert(m_AttributeDescriptions.end(),
			detail.attributeDescriptions.begin(),
			detail.attributeDescriptions.end()
		);
	}

	m_ColorWriteMask = 0;
	m_ColorWriteMask |= (state.colorWrites[state.R] * VK_COLOR_COMPONENT_R_BIT);
	m_ColorWriteMask |= (state.colorWrites[state.G] * VK_COLOR_COMPONENT_G_BIT);
	m_ColorWriteMask |= (state.colorWrites[state.B] * VK_COLOR_COMPONENT_B_BIT);
	m_ColorWriteMask |= (state.colorWrites[state.A] * VK_COLOR_COMPONENT_A_BIT);

	ASSERT_RESULT(KVulkanHelper::BlendFactorToVkBlendFactor(state.blendSrcFactor, m_ColorSrcBlendFactor));
	ASSERT_RESULT(KVulkanHelper::BlendFactorToVkBlendFactor(state.blendDstFactor, m_ColorDstBlendFactor));
	ASSERT_RESULT(KVulkanHelper::BlendOperatorToVkBlendOp(state.blendOp, m_ColorBlendOp));

	m_BlendEnable = state.blend;

	ASSERT_RESULT(KVulkanHelper::CullModeToVkCullMode(state.cullMode, m_CullMode));
	ASSERT_RESULT(KVulkanHelper::FrontFaceToVkFrontFace(state.frontFace, m_FrontFace));
	ASSERT_RESULT(KVulkanHelper::PolygonModeToVkPolygonMode(state.polygonMode, m_PolygonMode));
	ASSERT_RESULT(KVulkanHelper::CompareFuncToVkCompareOp(state.depthComp, m_DepthCompareOp));

	m_DepthWrite = (VkBool32)state.depthWrite;
	m_DepthTest = (VkBool32)state.depthTest;

	m_DepthBiasEnable = (VkBool32)state.depthBias;

	ASSERT_RESULT(KVulkanHelper::CompareFuncToVkCompareOp(state.stencilComp, m_StencilCompareOp));
	ASSERT_RESULT(KVulkanHelper::StencilOperatorToVkStencilOp(state.stencilFailOp, m_StencilFailOp));
	ASSERT_RESULT(KVulkanHelper::StencilOperatorToVkStencilOp(state.stencilDepthFailOp, m_StencilDepthFailOp));
	ASSERT_RESULT(KVulkanHelper::StencilOperatorToVkStencilOp(state.stencilPassOp, m_StencilPassOp));

	m_StencilRef = state.stencilRef;
	m_StencilEnable = (VkBool32)state.stencil;
	return true;
}

bool KVulkanPipelineHandle::Init(IKPipelineLayout* layout, IKRenderPass* renderPass, const KPipelineState& state, const KPipelineBinding& binding)
{
	UnInit();

	ASSERT_RESULT(layout);
	ASSERT_RESULT(renderPass);

	ASSERT_RESULT(m_GraphicsPipeline == VK_NULL_HANDLE);

	KVulkanPipelineLayout* vulkanPipelineLayout = static_cast<KVulkanPipelineLayout*>(layout);
	KVulkanRenderPass* vulkanRenderPass = static_cast<KVulkanRenderPass*>(renderPass);

	VkSampleCountFlagBits msaaFlag = vulkanRenderPass->GetMSAAFlag();
	const KViewPortArea& area = vulkanRenderPass->GetViewPort();

	InitializeVulkanTerminology(state, binding);

	// 配置顶点输入信息
	VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

	vertexInputInfo.vertexBindingDescriptionCount = (uint32_t)m_BindingDescriptions.size();
	vertexInputInfo.pVertexBindingDescriptions = m_BindingDescriptions.data();
	vertexInputInfo.vertexAttributeDescriptionCount = (uint32_t)m_AttributeDescriptions.size();
	vertexInputInfo.pVertexAttributeDescriptions = m_AttributeDescriptions.data();

	// 配置顶点组装信息
	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = m_PrimitiveTopology;
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
	rasterizer.polygonMode = m_PolygonMode;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = m_CullMode;
	rasterizer.frontFace = m_FrontFace;

	rasterizer.depthBiasEnable = m_DepthBiasEnable;
	rasterizer.depthBiasConstantFactor = 0.0f; // Optional
	rasterizer.depthBiasClamp = 0.0f; // Optional
	rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

	// 配置深度缓冲信息
	VkPipelineDepthStencilStateCreateInfo depthStencil = {};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = m_DepthTest;
	depthStencil.depthWriteEnable = m_DepthWrite;
	depthStencil.depthCompareOp = m_DepthCompareOp;

	depthStencil.depthBoundsTestEnable = VK_FALSE; // Optional
	depthStencil.minDepthBounds = 0.0f; // Optional
	depthStencil.maxDepthBounds = 1.0f; // Optional

	depthStencil.stencilTestEnable = m_StencilEnable;

	VkStencilOpState stencilState = {};
	if (m_StencilEnable)
	{
		stencilState.compareMask = 0xFF;
		stencilState.writeMask = 0xFF;

		stencilState.compareOp = m_StencilCompareOp;
		stencilState.failOp = m_StencilFailOp;
		stencilState.depthFailOp = m_StencilDepthFailOp;
		stencilState.passOp = m_StencilPassOp;
		stencilState.reference = m_StencilRef;
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
	colorBlendAttachment.colorWriteMask = m_ColorWriteMask;
	colorBlendAttachment.blendEnable = m_BlendEnable;

	colorBlendAttachment.srcColorBlendFactor = m_ColorSrcBlendFactor;
	colorBlendAttachment.dstColorBlendFactor = m_ColorDstBlendFactor;
	colorBlendAttachment.colorBlendOp = m_ColorBlendOp;

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

	static VkShaderStageFlagBits SHADER_STAGE_FLAGS[] =
	{
		VK_SHADER_STAGE_VERTEX_BIT,
		VK_SHADER_STAGE_FRAGMENT_BIT,
		VK_SHADER_STAGE_GEOMETRY_BIT,
		VK_SHADER_STAGE_TASK_BIT_NV,
		VK_SHADER_STAGE_MESH_BIT_NV
	};

	static_assert(ARRAY_SIZE(SHADER_STAGE_FLAGS) == LAYOUT_SHADER_COUNT, "must match");

	for (uint32_t i = 0; i < LAYOUT_SHADER_COUNT; ++i)
	{
		if (binding.shaders[i])
		{
			VkPipelineShaderStageCreateInfo shaderCreateInfo = {};
			vulkanShader = (KVulkanShader*)binding.shaders[i].get();
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
	pipelineInfo.layout = vulkanPipelineLayout->GetPipelineLayout();
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

bool KVulkanPipelineHandle::SetDebugName(const char* name)
{
	if (name)
	{
		m_Name = name;
		KVulkanHelper::DebugUtilsSetObjectName(KVulkanGlobal::device, (uint64_t)m_GraphicsPipeline, VK_OBJECT_TYPE_PIPELINE, name);
		return true;
	}
	return false;
}