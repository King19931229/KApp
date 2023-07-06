#include "KVulkanCommandBuffer.h"
#include "KVulkanRenderTarget.h"
#include "KVulkanRenderPass.h"
#include "KVulkanPipeline.h"
#include "KVulkanBuffer.h"
#include "KVulkanFrameBuffer.h"
#include "KVulkanQuery.h"
#include "KVulkanGlobal.h"
#include "KVulkanInitializer.h"
#include "KVulkanHelper.h"
#include "Internal/KConstantDefinition.h"

KVulkanCommandPool::KVulkanCommandPool()
	: m_ResetMode(CBR_RESET_POOL)
{
}

KVulkanCommandPool::~KVulkanCommandPool()
{
	assert(m_CommandPools.empty());
}

bool KVulkanCommandPool::Init(QueueCategory queue, uint32_t index, CommmandBufferReset resetMode)
{
	ASSERT_RESULT(UnInit());

	ASSERT_RESULT(KVulkanGlobal::deviceReady);

	const std::vector<uint32_t>* queueFamilyIndices = nullptr;

	switch (queue)
	{
		case QUEUE_GRAPHICS:
		case QUEUE_PRESENT:
			queueFamilyIndices = &KVulkanGlobal::graphicsFamilyIndices;
			break;
		case QUEUE_COMPUTE:
			queueFamilyIndices = &KVulkanGlobal::computeFamilyIndices;
			break;
		case QUEUE_TRANSFER:
			queueFamilyIndices = &KVulkanGlobal::transferFamilyIndices;
			break;
		default:
			assert(false && "impossible to reach");
			break;
	}

	assert(queueFamilyIndices && index < queueFamilyIndices->size());
	if (!queueFamilyIndices || index >= queueFamilyIndices->size())
	{
		return false;
	}

	VkCommandPoolCreateFlags flags = 0;

	switch (resetMode)
	{
		case CBR_RESET_INDIVIDUALLY:
			flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
			break;
		case CBR_RESET_ALLOCATE_FREE:
		case CBR_RESET_POOL:
		default:
			flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
			break;
	}

	m_CommandPools.resize(KRenderGlobal::NumFramesInFlight);
	for (size_t i = 0; i < m_CommandPools.size(); ++i)
	{
		VkCommandPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.queueFamilyIndex = (*queueFamilyIndices)[index];
		poolInfo.flags = flags;
		VK_ASSERT_RESULT(vkCreateCommandPool(KVulkanGlobal::device, &poolInfo, nullptr, &m_CommandPools[i]));
	}

	m_ResetMode = resetMode;

	return true;	
}

bool KVulkanCommandPool::UnInit()
{
	Reset();

	if(m_CommandPools.size() > 0)
	{
		for (size_t i = 0; i < m_CommandPools.size(); ++i)
		{
			ASSERT_RESULT(KVulkanGlobal::deviceReady);
			vkDestroyCommandPool(KVulkanGlobal::device, m_CommandPools[i], nullptr);
		}
		m_CommandPools.clear();
		return true;
	}
	return true;
}

IKCommandBufferPtr KVulkanCommandPool::Request(CommandBufferLevel level)
{
	IKCommandBufferPtr buffer = nullptr;

	BufferUsage& usage = level == CBL_PRIMARY ? m_PrimaryUsage : m_SecondaryUsage;

	if (usage.currentActive < usage.buffers.size())
	{
		buffer = usage.buffers[usage.currentActive++];
	}
	else
	{
		KVulkanCommandBuffer* vulkanCommandBuffer = KNEW KVulkanCommandBuffer();
		vulkanCommandBuffer->Init(this, level);
		if (!m_Name.empty())
		{
			vulkanCommandBuffer->SetDebugName((m_Name + "_CommandBuffer_" + std::to_string(usage.buffers.size())).c_str());
		}
		buffer = IKCommandBufferPtr(vulkanCommandBuffer);
		usage.buffers.push_back(buffer);
		++usage.currentActive;
	}

	return buffer;
}

bool KVulkanCommandPool::SetDebugName(const char* name)
{
	m_Name = name;
	if (m_CommandPools.size() > 0)
	{
		for (size_t i = 0; i < m_CommandPools.size(); ++i)
		{
			KVulkanHelper::DebugUtilsSetObjectName(KVulkanGlobal::device, (uint64_t)m_CommandPools[i], VK_OBJECT_TYPE_COMMAND_POOL, (name + std::string("_") + std::to_string(i)).c_str());
		}
		return true;
	}
	return false;
}

void KVulkanCommandPool::BufferUsage::Reset(CommmandBufferReset resetMode)
{
	if (resetMode == CBR_RESET_INDIVIDUALLY)
	{
		for (IKCommandBufferPtr buffer : buffers)
		{
			((KVulkanCommandBuffer*)(buffer.get()))->Reset(resetMode);
		}
	}
	else if(resetMode == CBR_RESET_ALLOCATE_FREE)
	{
		for (IKCommandBufferPtr buffer : buffers)
		{
			((KVulkanCommandBuffer*)(buffer.get()))->UnInit();
		}
		buffers.clear();
	}
	currentActive = 0;
}

bool KVulkanCommandPool::Reset()
{
	if (m_ResetMode == CBR_RESET_POOL)
	{
		if (m_CommandPools.size() > 0)
		{
			ASSERT_RESULT(KVulkanGlobal::deviceReady);
			VK_ASSERT_RESULT(vkResetCommandPool(KVulkanGlobal::device, m_CommandPools[KRenderGlobal::CurrentInFlightFrameIndex], 0));
		}
	}

	m_PrimaryUsage.Reset(m_ResetMode);
	m_SecondaryUsage.Reset(m_ResetMode);

	return false;
}

KVulkanCommandBuffer::KVulkanCommandBuffer()
	: m_CommandLevel(VK_COMMAND_BUFFER_LEVEL_MAX_ENUM)
{
}

KVulkanCommandBuffer::~KVulkanCommandBuffer()
{
}

bool KVulkanCommandBuffer::Init(KVulkanCommandPool* pool, CommandBufferLevel level)
{
	ASSERT_RESULT(UnInit());

	ASSERT_RESULT(KVulkanGlobal::deviceReady);
	KVulkanCommandPool* vulkanPool = (KVulkanCommandPool*)pool;

	m_ParentPools = vulkanPool->GetVkHandles();
	ASSERT_RESULT(!m_ParentPools.empty());

	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;

	switch (level)
	{
	case CBL_PRIMARY:
		m_CommandLevel = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		break;
	case CBL_SECONDARY:
		m_CommandLevel = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
		break;
	default:
		assert(false && "impossible to reach");
		break;
	}

	allocInfo.level = m_CommandLevel;
	allocInfo.commandBufferCount = 1;
	m_CommandBuffers.resize(KRenderGlobal::NumFramesInFlight);

	for (size_t i = 0; i < m_CommandBuffers.size(); ++i)
	{
		allocInfo.commandPool = m_ParentPools[i];
		VK_ASSERT_RESULT(vkAllocateCommandBuffers(KVulkanGlobal::device, &allocInfo, &m_CommandBuffers[i]));
	}
	return true;
}

bool KVulkanCommandBuffer::UnInit()
{
	if (m_CommandBuffers.size())
	{
		ASSERT_RESULT(KVulkanGlobal::deviceReady);
		for (size_t i = 0; i < m_CommandBuffers.size(); ++i)
		{
			vkFreeCommandBuffers(KVulkanGlobal::device, m_ParentPools[i], 1, &m_CommandBuffers[i]);
		}
		m_CommandBuffers.clear();
	}

	m_ParentPools.clear();

	m_CommandLevel = VK_COMMAND_BUFFER_LEVEL_MAX_ENUM;

	return true;
}

bool KVulkanCommandBuffer::Reset(CommmandBufferReset resetMode)
{
	if (resetMode == CBR_RESET_INDIVIDUALLY)
	{
		if (m_CommandBuffers.size() > 0)
		{
			VK_ASSERT_RESULT(vkResetCommandBuffer(m_CommandBuffers[KRenderGlobal::CurrentInFlightFrameIndex], VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT));
		}
	}
	return true;
}

bool KVulkanCommandBuffer::SetDebugName(const char* name)
{
	if (m_CommandBuffers.size() > 0)
	{
		for (size_t i = 0; i < m_CommandBuffers.size(); ++i)
		{
			KVulkanHelper::DebugUtilsSetObjectName(KVulkanGlobal::device, (uint64_t)m_CommandBuffers[i], VK_OBJECT_TYPE_COMMAND_BUFFER, (name + std::string("_FrameID_") + std::to_string(i)).c_str());
		}
		return true;
	}
	return false;
}

VkCommandBuffer KVulkanCommandBuffer::GetVkHandle()
{
	assert(KRenderGlobal::CurrentInFlightFrameIndex < m_CommandBuffers.size());
	return KRenderGlobal::CurrentInFlightFrameIndex < m_CommandBuffers.size() ? m_CommandBuffers[KRenderGlobal::CurrentInFlightFrameIndex] : VK_NULL_HANDEL;
}

bool KVulkanCommandBuffer::SetViewport(const KViewPortArea& area)
{	
	VkCommandBuffer commandBuffer = GetVkHandle();

	if (commandBuffer != VK_NULL_HANDLE)
	{
		// 设置视口与裁剪
		VkOffset2D offset = { (int32_t)area.x, (int32_t)area.y };
		VkExtent2D extent = { area.width, area.height };

		VkRect2D scissorRect = { offset, extent };
		VkViewport viewPort =
		{
			(float)offset.x,
			(float)offset.y,
			(float)extent.width,
			(float)extent.height,
			0.0f,
			1.0f
		};

		vkCmdSetViewport(commandBuffer, 0, 1, &viewPort);
		vkCmdSetScissor(commandBuffer, 0, 1, &scissorRect);

		return true;
	}
	return false;
}

bool KVulkanCommandBuffer::SetDepthBias(float depthBiasConstant, float depthBiasClamp, float depthBiasSlope)
{
	VkCommandBuffer commandBuffer = GetVkHandle();
	if(commandBuffer != VK_NULL_HANDLE)
	{
		vkCmdSetDepthBias(commandBuffer, depthBiasConstant, depthBiasClamp, depthBiasSlope);
		return true;
	}
	return false;
}

bool KVulkanCommandBuffer::Render(const KRenderCommand& command)
{
	VkCommandBuffer commandBuffer = GetVkHandle();
	if(commandBuffer != VK_NULL_HANDLE)
	{
		if(!command.Complete())
		{
			assert(false && "render command is not complete. check the logic");
			return false;
		}

		KVulkanPipeline* vulkanPipeline = (KVulkanPipeline*)command.pipeline.get();
		KVulkanPipelineHandle* pipelineHandle = (KVulkanPipelineHandle*)command.pipelineHandle.get();

		VkPipeline pipeline = pipelineHandle->GetVkPipeline();
		VkPipelineLayout pipelineLayout = vulkanPipeline->GetVkPipelineLayout();

		const KDynamicConstantBufferUsage* dynamicUsages[CBT_DYNAMIC_COUNT] = {};
		uint32_t dynamicOffsets[CBT_DYNAMIC_COUNT] = { 0 };
		uint32_t dynamicBufferCount = 0;

		if (command.objectUsage.buffer)
		{
			dynamicUsages[dynamicBufferCount++] = &(command.objectUsage);
		}
		if (command.shadingUsage.buffer)
		{
			dynamicUsages[dynamicBufferCount++] = &(command.shadingUsage);
		}
		if (command.debugUsage.buffer)
		{
			dynamicUsages[dynamicBufferCount++] = &(command.debugUsage);
		}

		for (uint32_t i = 0; i < dynamicBufferCount; ++i)
		{
			dynamicOffsets[i] = (uint32_t)dynamicUsages[i]->offset;
		}

		const KStorageBufferUsage* storageUsages[SBT_COUNT] = {};
		uint32_t storageBufferCount = 0;
		for (size_t i = 0; i < command.meshStorageUsages.size(); ++i)
		{
			storageUsages[storageBufferCount++] = &(command.meshStorageUsages[i]);
		}

		VkDescriptorSet descriptorSet = vulkanPipeline->AllocDescriptorSet(command.threadIndex, dynamicUsages, dynamicBufferCount, storageUsages, storageBufferCount);

		// 绑定管线
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
		// 绑定管线布局
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, dynamicBufferCount, dynamicOffsets);

		// 绑定顶点缓冲
		VkBuffer vertexBuffers[32] = {0};
		VkDeviceSize offsets[32] = {0};

		uint32_t vertexBufferCount = static_cast<uint32_t>(command.vertexData->vertexBuffers.size());

		assert(vertexBufferCount < 32);

		for(uint32_t i = 0; i < vertexBufferCount; ++i)
		{
			IKVertexBufferPtr vertexBuffer = command.vertexData->vertexBuffers[i];
			ASSERT_RESULT(vertexBuffer);
			vertexBuffers[i] = ((KVulkanVertexBuffer*)vertexBuffer.get())->GetVulkanHandle();
			offsets[i] = 0;
		}

		if (command.meshShaderDraw)
		{
			KVulkanGlobal::vkCmdDrawMeshTasksNV(commandBuffer, command.meshData->count, command.meshData->offset);
		}
		else
		{
			if (command.instanceDraw)
			{
				uint32_t instanceSlot = vertexBufferCount++;

				for (const KInstanceBufferUsage& instanceUsage : command.instanceUsages)
				{
					IKVertexBufferPtr instanceBuffer = instanceUsage.buffer;
					ASSERT_RESULT(instanceBuffer);
					vertexBuffers[instanceSlot] = ((KVulkanVertexBuffer*)instanceBuffer.get())->GetVulkanHandle();
					offsets[instanceSlot] = 0;
					vkCmdBindVertexBuffers(commandBuffer, 0, vertexBufferCount, vertexBuffers, offsets);

					uint32_t instanceStart = static_cast<uint32_t>(instanceUsage.start);
					uint32_t instanceCount = static_cast<uint32_t>(instanceUsage.count);

					if (command.indexDraw)
					{
						KVulkanIndexBuffer* vulkanIndexBuffer = ((KVulkanIndexBuffer*)command.indexData->indexBuffer.get());
						vkCmdBindIndexBuffer(commandBuffer, vulkanIndexBuffer->GetVulkanHandle(), 0, vulkanIndexBuffer->GetVulkanIndexType());
						vkCmdDrawIndexed(commandBuffer, command.indexData->indexCount, instanceCount, command.indexData->indexStart, 0, instanceStart);
					}
					else
					{
						vkCmdDraw(commandBuffer, command.vertexData->vertexCount, instanceCount, command.vertexData->vertexStart, instanceStart);
					}
				}
			}
			else
			{
				if (vertexBufferCount > 0)
				{
					vkCmdBindVertexBuffers(commandBuffer, 0, vertexBufferCount, vertexBuffers, offsets);
				}

				if (command.indexDraw)
				{
					KVulkanIndexBuffer* vulkanIndexBuffer = ((KVulkanIndexBuffer*)command.indexData->indexBuffer.get());
					vkCmdBindIndexBuffer(commandBuffer, vulkanIndexBuffer->GetVulkanHandle(), 0, vulkanIndexBuffer->GetVulkanIndexType());
					vkCmdDrawIndexed(commandBuffer, command.indexData->indexCount, 1, command.indexData->indexStart, 0, 0);
				}
				else
				{
					vkCmdDraw(commandBuffer, command.vertexData->vertexCount, 1, command.vertexData->vertexStart, 0);
				}
			}
		}

		return true;
	}
	return false;
}

bool KVulkanCommandBuffer::Execute(IKCommandBufferPtr buffer)
{
	VkCommandBuffer commandBuffer = GetVkHandle();
	assert(commandBuffer != VK_NULL_HANDLE);
	assert(m_CommandLevel == VK_COMMAND_BUFFER_LEVEL_PRIMARY);

	VkCommandBuffer handle = ((KVulkanCommandBuffer*)buffer.get())->GetVkHandle();
	VkCommandBufferLevel level = ((KVulkanCommandBuffer*)buffer.get())->GetVkBufferLevel();
	if(handle != VK_NULL_HANDLE && level == VK_COMMAND_BUFFER_LEVEL_SECONDARY)
	{
		vkCmdExecuteCommands(commandBuffer, 1, &handle);
		return true;
	}
	return false;
}

bool KVulkanCommandBuffer::ExecuteAll(KCommandBufferList& commandBuffers, bool clearAfterExecute)
{
	VkCommandBuffer commandBuffer = GetVkHandle();
	assert(commandBuffer != VK_NULL_HANDLE);
	assert(m_CommandLevel == VK_COMMAND_BUFFER_LEVEL_PRIMARY);

	if (commandBuffer != VK_NULL_HANDLE && m_CommandLevel == VK_COMMAND_BUFFER_LEVEL_PRIMARY)
	{
		if (commandBuffers.size() > 0)
		{
			std::vector<VkCommandBuffer> vkCommandBuffers;
			for (IKCommandBufferPtr buffer : commandBuffers)
			{
				VkCommandBuffer handle = ((KVulkanCommandBuffer*)buffer.get())->GetVkHandle();
				VkCommandBufferLevel level = ((KVulkanCommandBuffer*)buffer.get())->GetVkBufferLevel();
				ASSERT_RESULT(handle != VK_NULL_HANDLE && level == VK_COMMAND_BUFFER_LEVEL_SECONDARY);
				if (handle != VK_NULL_HANDLE && level == VK_COMMAND_BUFFER_LEVEL_SECONDARY)
				{
					vkCommandBuffers.push_back(handle);
				}
			}
			vkCmdExecuteCommands(commandBuffer, (uint32_t)vkCommandBuffers.size(), vkCommandBuffers.data());
		}
		if (clearAfterExecute)
		{
			commandBuffers.clear();
		}
		return true;
	}

	return false;
}

bool KVulkanCommandBuffer::BeginPrimary()
{
	VkCommandBuffer commandBuffer = GetVkHandle();
	assert(commandBuffer != VK_NULL_HANDLE);
	assert(m_CommandLevel == VK_COMMAND_BUFFER_LEVEL_PRIMARY);

	if(commandBuffer != VK_NULL_HANDLE && m_CommandLevel == VK_COMMAND_BUFFER_LEVEL_PRIMARY)
	{
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		VK_ASSERT_RESULT(vkBeginCommandBuffer(commandBuffer, &beginInfo));
		return true;
	}

	return false;
}

bool KVulkanCommandBuffer::BeginSecondary(IKRenderPassPtr renderPass)
{
	VkCommandBuffer commandBuffer = GetVkHandle();
	assert(commandBuffer != VK_NULL_HANDLE);
	assert(m_CommandLevel == VK_COMMAND_BUFFER_LEVEL_SECONDARY);

	if (commandBuffer != VK_NULL_HANDLE && m_CommandLevel == VK_COMMAND_BUFFER_LEVEL_SECONDARY)
	{
		KVulkanRenderPass* vulkanRenderPass = (KVulkanRenderPass*)renderPass.get();

		VkCommandBufferInheritanceInfo inheritanceInfo = {};
		inheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
		inheritanceInfo.renderPass = vulkanRenderPass->GetVkRenderPass();
		inheritanceInfo.framebuffer = vulkanRenderPass->GetVkFrameBuffer();
		inheritanceInfo.occlusionQueryEnable = VK_TRUE;
		inheritanceInfo.queryFlags = 0;

		// 命令开始时候创建需要一个命令开始信息
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT | VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
		beginInfo.pInheritanceInfo = &inheritanceInfo;

		VK_ASSERT_RESULT(vkBeginCommandBuffer(commandBuffer, &beginInfo));

		return true;
	}
	return false;
}

bool KVulkanCommandBuffer::BeginRenderPass(IKRenderPassPtr renderPass, SubpassContents conent)
{
	VkCommandBuffer commandBuffer = GetVkHandle();
	assert(renderPass);
	assert(commandBuffer != VK_NULL_HANDLE);
	if (commandBuffer != VK_NULL_HANDLE)
	{
		KVulkanRenderPass* vulkanRenderPass = (KVulkanRenderPass*)renderPass.get();

		VkRenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		// 指定渲染通道
		renderPassInfo.renderPass = vulkanRenderPass->GetVkRenderPass();
		// 指定帧缓冲
		renderPassInfo.framebuffer = vulkanRenderPass->GetVkFrameBuffer();

		const KViewPortArea& area = renderPass->GetViewPort();
		renderPassInfo.renderArea.offset = { (int32_t)area.x, (int32_t)area.y };
		renderPassInfo.renderArea.extent = { area.width, area.height };

		KVulkanRenderPass::VkClearValueArray clearValues;
		vulkanRenderPass->GetVkClearValues(clearValues);
		// 注意清理缓冲值的顺序要和RenderPass绑定Attachment的顺序一致

		renderPassInfo.pClearValues = clearValues.data();
		renderPassInfo.clearValueCount = (uint32_t)clearValues.size();

		VkSubpassContents subpassContents = VK_SUBPASS_CONTENTS_MAX_ENUM;
		switch (conent)
		{
			case SUBPASS_CONTENTS_INLINE:
				subpassContents = VK_SUBPASS_CONTENTS_INLINE;
				break;
			case SUBPASS_CONTENTS_SECONDARY:
				subpassContents = VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS;
				break;
			default:
				assert(false && "unable to reach");
				break;
		}

		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, subpassContents);

		return true;
	}
	return false;
}

bool KVulkanCommandBuffer::ClearColor(uint32_t attachment, const KViewPortArea& area, const KClearColor& color)
{
	VkCommandBuffer commandBuffer = GetVkHandle();
	VkClearValue clearValue;
	VkClearAttachment clearAttachment;

	clearValue.color.float32[0] = color.r;
	clearValue.color.float32[1] = color.g;
	clearValue.color.float32[2] = color.b;
	clearValue.color.float32[3] = color.a;

	clearAttachment.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	clearAttachment.colorAttachment = attachment;
	clearAttachment.clearValue = clearValue;

	VkClearRect clearRect;
	clearRect.baseArrayLayer = 0;
	clearRect.layerCount = 1;
	clearRect.rect.offset = { (int32_t)area.x, (int32_t)area.y };
	clearRect.rect.extent = { area.width, area.height };

	vkCmdClearAttachments(commandBuffer, 1, &clearAttachment, 1, &clearRect);

	return true;
}

bool KVulkanCommandBuffer::ClearDepthStencil(const KViewPortArea& area, const KClearDepthStencil& depthStencil)
{
	VkCommandBuffer commandBuffer = GetVkHandle();
	VkClearValue clearValue;
	VkClearAttachment clearAttachment;

	clearValue.depthStencil.depth = depthStencil.depth;
	clearValue.depthStencil.stencil = depthStencil.stencil;

	clearAttachment.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
	clearAttachment.colorAttachment = 0;
	clearAttachment.clearValue = clearValue;

	VkClearRect clearRect;
	clearRect.baseArrayLayer = 0;
	clearRect.layerCount = 1;
	clearRect.rect.offset = { (int32_t)area.x, (int32_t)area.y };
	clearRect.rect.extent = { area.width, area.height };

	vkCmdClearAttachments(commandBuffer, 1, &clearAttachment, 1, &clearRect);

	return true;
}

bool KVulkanCommandBuffer::EndRenderPass()
{
	VkCommandBuffer commandBuffer = GetVkHandle();
	assert(commandBuffer != VK_NULL_HANDLE);
	if(commandBuffer != VK_NULL_HANDLE)
	{
		vkCmdEndRenderPass(commandBuffer);
		return true;
	}
	return false;
}

bool KVulkanCommandBuffer::BeginDebugMarker(const std::string& marker, const glm::vec4 color)
{
	VkCommandBuffer commandBuffer = GetVkHandle();
#ifdef VK_USE_DEBUG_UTILS_AS_DEBUG_MARKER
	KVulkanHelper::DebugUtilsBeginRegion(commandBuffer, marker.c_str(), color);
#else
	KVulkanHelper::DebugMarkerBeginRegion(commandBuffer, marker.c_str(), color);
#endif
	return true;
}

bool KVulkanCommandBuffer::EndDebugMarker()
{
	VkCommandBuffer commandBuffer = GetVkHandle();
#ifdef VK_USE_DEBUG_UTILS_AS_DEBUG_MARKER
	KVulkanHelper::DebugUtilsEndRegion(commandBuffer);
#else
	KVulkanHelper::DebugMarkerEndRegion(commandBuffer);
#endif
	return true;
}

bool KVulkanCommandBuffer::End()
{
	VkCommandBuffer commandBuffer = GetVkHandle();
	assert(commandBuffer != VK_NULL_HANDLE);
	if(commandBuffer != VK_NULL_HANDLE)
	{
		VK_ASSERT_RESULT(vkEndCommandBuffer(commandBuffer));
		return true;
	}
	return false;
}

bool KVulkanCommandBuffer::Flush()
{
	VkCommandBuffer commandBuffer = GetVkHandle();
	assert(commandBuffer != VK_NULL_HANDLE);
	if (commandBuffer != VK_NULL_HANDLE)
	{
		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		VkFenceCreateInfo fenceInfo = {};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FLAGS_NONE;
		fenceInfo.pNext = nullptr;

		VkFence fence;

		VK_ASSERT_RESULT(vkCreateFence(KVulkanGlobal::device, &fenceInfo, nullptr, &fence));
		// Wait for the fence to signal that command buffer has finished executing
		VK_ASSERT_RESULT(vkQueueSubmit(KVulkanGlobal::graphicsQueues[0], 1, &submitInfo, fence));
		VK_ASSERT_RESULT(vkWaitForFences(KVulkanGlobal::device, 1, &fence, VK_TRUE, DEFAULT_FENCE_TIMEOUT));
		vkDestroyFence(KVulkanGlobal::device, fence, nullptr);

		return true;
	}
	return false;
}

bool KVulkanCommandBuffer::BeginQuery(IKQueryPtr query)
{
	VkCommandBuffer commandBuffer = GetVkHandle();
	if (query)
	{
		KVulkanQuery* vulkanQuery = (KVulkanQuery*)query.get();
		vulkanQuery->Begin(commandBuffer);
		return true;
	}
	return false;
}

bool KVulkanCommandBuffer::EndQuery(IKQueryPtr query)
{
	VkCommandBuffer commandBuffer = GetVkHandle();
	if (query)
	{
		KVulkanQuery* vulkanQuery = (KVulkanQuery*)query.get();
		vulkanQuery->End(commandBuffer);
		return true;
	}
	return false;
}

bool KVulkanCommandBuffer::ResetQuery(IKQueryPtr query)
{
	VkCommandBuffer commandBuffer = GetVkHandle();
	if (query)
	{
		KVulkanQuery* vulkanQuery = (KVulkanQuery*)query.get();
		vulkanQuery->Reset(commandBuffer);
		return true;
	}
	return false;
}

bool KVulkanCommandBuffer::Translate(IKFrameBufferPtr buf, PipelineStages srcStages, PipelineStages dstStages, ImageLayout oldLayout, ImageLayout newLayout)
{
	if (buf)
	{
		return buf->Translate(this, nullptr, nullptr, 0, buf->GetMipmaps(), srcStages, dstStages, oldLayout, newLayout);
	}
	return false;
}

bool KVulkanCommandBuffer::TranslateOwnership(IKFrameBufferPtr buf, IKQueuePtr srcQueue, IKQueuePtr dstQueue, PipelineStages srcStages, PipelineStages dstStages, ImageLayout oldLayout, ImageLayout newLayout)
{
	if (buf)
	{
		return buf->Translate(this, srcQueue.get(), dstQueue.get(), 0, buf->GetMipmaps(), srcStages, dstStages, oldLayout, newLayout);
	}
	return false;
}

bool KVulkanCommandBuffer::TranslateMipmap(IKFrameBufferPtr buf, uint32_t mipmap, PipelineStages srcStages, PipelineStages dstStages, ImageLayout oldLayout, ImageLayout newLayout)
{
	if (buf)
	{
		return buf->Translate(this, nullptr, nullptr, mipmap, 1, srcStages, dstStages, oldLayout, newLayout);
	}
	return false;
}

bool KVulkanCommandBuffer::Blit(IKFrameBufferPtr src, IKFrameBufferPtr dest)
{
	VkCommandBuffer commandBuffer = GetVkHandle();
	if (src && dest)
	{
		KVulkanFrameBuffer* srcBuffer = (KVulkanFrameBuffer*)src.get();
		KVulkanFrameBuffer* destBuffer = (KVulkanFrameBuffer*)dest.get();

		VkImageBlit blit = {};

		VkOffset3D srcOffsets[] = { { 0, 0, 0 },{ (int32_t)srcBuffer->GetWidth(), (int32_t)srcBuffer->GetHeight(), (int32_t)srcBuffer->GetDepth() } };
		blit.srcOffsets[0] = srcOffsets[0];
		blit.srcOffsets[1] = srcOffsets[1];

		blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.srcSubresource.mipLevel = 0;
		blit.srcSubresource.baseArrayLayer = 0;
		blit.srcSubresource.layerCount = 1;

		VkOffset3D dstOffsets[] = { { 0, 0, 0 },{ (int32_t)destBuffer->GetWidth(), (int32_t)destBuffer->GetHeight(), (int32_t)destBuffer->GetDepth() } };
		blit.dstOffsets[0] = dstOffsets[0];
		blit.dstOffsets[1] = dstOffsets[1];

		blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.dstSubresource.mipLevel = 0;
		blit.dstSubresource.baseArrayLayer = 0;
		blit.dstSubresource.layerCount = 1;

		vkCmdBlitImage(commandBuffer,
			srcBuffer->GetImage(), srcBuffer->GetImageLayout(),
			destBuffer->GetImage(), destBuffer->GetImageLayout(),
			1, &blit,
			VK_FILTER_NEAREST);

		return true;
	}
	return false;
}