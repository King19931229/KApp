#include "KVulkanCommandBuffer.h"
#include "KVulkanRenderTarget.h"
#include "KVulkanPipeline.h"
#include "KVulkanBuffer.h"
#include "KVulkanQuery.h"
#include "KVulkanGlobal.h"
#include "Internal/KConstantDefinition.h"

KVulkanCommandPool::KVulkanCommandPool()
	: m_CommandPool(VK_NULL_HANDLE)
{

}

KVulkanCommandPool::~KVulkanCommandPool()
{
	assert(m_CommandPool == VK_NULL_HANDLE);
}

bool KVulkanCommandPool::Init(QueueFamilyIndex familyIndex)
{
	ASSERT_RESULT(UnInit());

	ASSERT_RESULT(KVulkanGlobal::deviceReady);
	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;

	switch (familyIndex)
	{
	case QUEUE_FAMILY_INDEX_GRAPHICS:
		poolInfo.queueFamilyIndex = KVulkanGlobal::graphicsFamilyIndex;
		break;
	case QUEUE_FAMILY_INDEX_PRESENT:
		poolInfo.queueFamilyIndex = KVulkanGlobal::presentFamilyIndex;
		break;
	default:
		assert(false && "impossible to reach");
		break;
	}
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	VK_ASSERT_RESULT(vkCreateCommandPool(KVulkanGlobal::device, &poolInfo, nullptr, &m_CommandPool));

	return true;	
}

bool KVulkanCommandPool::UnInit()
{
	if(m_CommandPool != VK_NULL_HANDLE)
	{
		ASSERT_RESULT(KVulkanGlobal::deviceReady);
		vkDestroyCommandPool(KVulkanGlobal::device, m_CommandPool, nullptr);
		m_CommandPool = VK_NULL_HANDLE;
		return true;
	}
	return true;
}

bool KVulkanCommandPool::Reset()
{
	if(m_CommandPool != VK_NULL_HANDLE)
	{
		ASSERT_RESULT(KVulkanGlobal::deviceReady);
		VK_ASSERT_RESULT(vkResetCommandPool(KVulkanGlobal::device, m_CommandPool, VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT));
		return true;
	}
	return false;
}

KVulkanCommandBuffer::KVulkanCommandBuffer()
	: m_CommandBuffer(VK_NULL_HANDLE),
	m_ParentPool(VK_NULL_HANDLE),
	m_CommandLevel(VK_COMMAND_BUFFER_LEVEL_MAX_ENUM)
{

}

KVulkanCommandBuffer::~KVulkanCommandBuffer()
{

}

bool KVulkanCommandBuffer::Init(IKCommandPoolPtr pool, CommandBufferLevel level)
{
	ASSERT_RESULT(UnInit());

	ASSERT_RESULT(KVulkanGlobal::deviceReady);
	KVulkanCommandPool* vulkanPool = (KVulkanCommandPool*)pool.get();

	m_ParentPool = vulkanPool->GetVkHandle();
	ASSERT_RESULT(m_ParentPool != VK_NULL_HANDLE);

	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = m_ParentPool;

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

	VK_ASSERT_RESULT(vkAllocateCommandBuffers(KVulkanGlobal::device, &allocInfo, &m_CommandBuffer));

	return true;
}

bool KVulkanCommandBuffer::UnInit()
{
	if(m_CommandBuffer != VK_NULL_HANDLE && m_ParentPool != VK_NULL_HANDLE)
	{
		ASSERT_RESULT(KVulkanGlobal::deviceReady);
		vkFreeCommandBuffers(KVulkanGlobal::device, m_ParentPool, 1, &m_CommandBuffer);
	}

	m_CommandBuffer = VK_NULL_HANDLE;
	m_ParentPool = VK_NULL_HANDLE;
	m_CommandLevel = VK_COMMAND_BUFFER_LEVEL_MAX_ENUM;

	return true;
}

bool KVulkanCommandBuffer::SetViewport(IKRenderTargetPtr target)
{
	assert(m_CommandBuffer != VK_NULL_HANDLE);
	if(m_CommandBuffer != VK_NULL_HANDLE)
	{
		KVulkanRenderTarget* vulkanTarget = (KVulkanRenderTarget*)target.get();

		// 设置视口与裁剪
		VkOffset2D offset = {0, 0};
		VkExtent2D extent = vulkanTarget->GetExtend();

		VkRect2D scissorRect = { offset, extent};
		VkViewport viewPort = 
		{
			0.0f,
			0.0f,
			(float)extent.width,
			(float)extent.height,
			0.0f,
			1.0f 
		};
		vkCmdSetViewport(m_CommandBuffer, 0, 1, &viewPort);
		vkCmdSetScissor(m_CommandBuffer, 0, 1, &scissorRect);

		return true;
	}
	return false;
}

bool KVulkanCommandBuffer::SetDepthBias(float depthBiasConstant, float depthBiasClamp, float depthBiasSlope)
{
	assert(m_CommandBuffer != VK_NULL_HANDLE);
	if(m_CommandBuffer != VK_NULL_HANDLE)
	{
		vkCmdSetDepthBias(m_CommandBuffer, depthBiasConstant, depthBiasClamp, depthBiasSlope);
		return true;
	}
	return false;
}

bool KVulkanCommandBuffer::Render(const KRenderCommand& command)
{
	assert(m_CommandBuffer != VK_NULL_HANDLE);
	if(m_CommandBuffer != VK_NULL_HANDLE)
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
		VkDescriptorSet descriptorSet = vulkanPipeline->GetVkDescriptorSet();

		// 绑定管线
		vkCmdBindPipeline(m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
		// 绑定管线布局
		vkCmdBindDescriptorSets(m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);

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

		if (command.instanceDraw)
		{
			IKVertexBufferPtr instanceBuffer = command.instanceBuffer;
			ASSERT_RESULT(instanceBuffer);
			vertexBuffers[vertexBufferCount] = ((KVulkanVertexBuffer*)instanceBuffer.get())->GetVulkanHandle();
			offsets[vertexBufferCount] = 0;
			++vertexBufferCount;
		}

		vkCmdBindVertexBuffers(m_CommandBuffer, 0, vertexBufferCount, vertexBuffers, offsets);

		if(!command.objectData.empty())
		{
			vkCmdPushConstants(m_CommandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, (uint32_t)command.objectData.size(), command.objectData.data());
		}

		if(command.indexDraw)
		{
			// 绑定索引缓冲
			KVulkanIndexBuffer* vulkanIndexBuffer = ((KVulkanIndexBuffer*)command.indexData->indexBuffer.get());
			vkCmdBindIndexBuffer(m_CommandBuffer, vulkanIndexBuffer->GetVulkanHandle(), 0, vulkanIndexBuffer->GetVulkanIndexType());

			if (command.instanceDraw)
			{
				vkCmdDrawIndexed(m_CommandBuffer, command.indexData->indexCount, command.instanceCount, command.indexData->indexStart, 0, 0);
			}
			else
			{
				vkCmdDrawIndexed(m_CommandBuffer, command.indexData->indexCount, 1, command.indexData->indexStart, 0, 0);
			}
		}
		else
		{
			if (command.instanceDraw)
			{
				vkCmdDraw(m_CommandBuffer, command.vertexData->vertexCount, command.instanceCount, command.vertexData->vertexStart, 0);
			}
			else
			{
				vkCmdDraw(m_CommandBuffer, command.vertexData->vertexCount, 1, command.vertexData->vertexStart, 0);
			}
		}

		return true;
	}
	return false;
}

bool KVulkanCommandBuffer::Execute(IKCommandBufferPtr buffer)
{
	assert(m_CommandBuffer != VK_NULL_HANDLE);
	assert(m_CommandLevel == VK_COMMAND_BUFFER_LEVEL_PRIMARY);

	VkCommandBuffer handle = ((KVulkanCommandBuffer*)buffer.get())->GetVkHandle();
	VkCommandBufferLevel level = ((KVulkanCommandBuffer*)buffer.get())->GetVkBufferLevel();
	if(handle != VK_NULL_HANDLE && level == VK_COMMAND_BUFFER_LEVEL_SECONDARY)
	{
		vkCmdExecuteCommands(m_CommandBuffer, 1, &handle);
		return true;
	}
	return false;
}

bool KVulkanCommandBuffer::ExecuteAll(KCommandBufferList& commandBuffers)
{
	assert(m_CommandBuffer != VK_NULL_HANDLE);
	assert(m_CommandLevel == VK_COMMAND_BUFFER_LEVEL_PRIMARY);
	assert(commandBuffers.size() > 0);

	if(m_CommandBuffer != VK_NULL_HANDLE && m_CommandLevel == VK_COMMAND_BUFFER_LEVEL_PRIMARY && commandBuffers.size() > 0)
	{
		std::vector<VkCommandBuffer> vkCommandBuffers;

		for(IKCommandBufferPtr buffer : commandBuffers)
		{
			VkCommandBuffer handle = ((KVulkanCommandBuffer*)buffer.get())->GetVkHandle();
			VkCommandBufferLevel level = ((KVulkanCommandBuffer*)buffer.get())->GetVkBufferLevel();
			ASSERT_RESULT(handle != VK_NULL_HANDLE && level == VK_COMMAND_BUFFER_LEVEL_SECONDARY);
			if(handle != VK_NULL_HANDLE && level == VK_COMMAND_BUFFER_LEVEL_SECONDARY)
			{
				vkCommandBuffers.push_back(handle);
			}
		}

		vkCmdExecuteCommands(m_CommandBuffer, (uint32_t)vkCommandBuffers.size(), vkCommandBuffers.data());

		return true;
	}
	return false;
}

bool KVulkanCommandBuffer::BeginPrimary()
{
	assert(m_CommandBuffer != VK_NULL_HANDLE);
	assert( m_CommandLevel == VK_COMMAND_BUFFER_LEVEL_PRIMARY);

	if(m_CommandBuffer != VK_NULL_HANDLE && m_CommandLevel == VK_COMMAND_BUFFER_LEVEL_PRIMARY)
	{
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		VK_ASSERT_RESULT(vkBeginCommandBuffer(m_CommandBuffer, &beginInfo));
		return true;
	}

	return false;
}

bool KVulkanCommandBuffer::BeginSecondary(IKRenderTargetPtr target)
{
	assert(m_CommandBuffer != VK_NULL_HANDLE);
	assert(m_CommandLevel == VK_COMMAND_BUFFER_LEVEL_SECONDARY);

	if(m_CommandBuffer != VK_NULL_HANDLE && m_CommandLevel == VK_COMMAND_BUFFER_LEVEL_SECONDARY)
	{
		KVulkanRenderTarget* vulkanTarget = (KVulkanRenderTarget*)target.get();

		VkCommandBufferInheritanceInfo inheritanceInfo = {};
		inheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
		inheritanceInfo.renderPass = vulkanTarget->GetRenderPass();
		inheritanceInfo.framebuffer = vulkanTarget->GetFrameBuffer();

		// 命令开始时候创建需要一个命令开始信息
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
		beginInfo.pInheritanceInfo = &inheritanceInfo;

		VK_ASSERT_RESULT(vkBeginCommandBuffer(m_CommandBuffer, &beginInfo));

		return true;
	}
	return false;
}

bool KVulkanCommandBuffer::BeginRenderPass(IKRenderTargetPtr target, SubpassContents conent, const KClearValue& clearValue)
{
	assert(target);
	assert(m_CommandBuffer != VK_NULL_HANDLE);
	if(m_CommandBuffer != VK_NULL_HANDLE)
	{
		KVulkanRenderTarget* vulkanTarget = (KVulkanRenderTarget*)target.get();

		VkRenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		// 指定渲染通道
		renderPassInfo.renderPass = vulkanTarget->GetRenderPass();
		// 指定帧缓冲
		renderPassInfo.framebuffer = vulkanTarget->GetFrameBuffer();

		renderPassInfo.renderArea.offset.x = 0;
		renderPassInfo.renderArea.offset.y = 0;
		renderPassInfo.renderArea.extent = vulkanTarget->GetExtend();

		// 注意清理缓冲值的顺序要和RenderPass绑定Attachment的顺序一致
		VkClearValue clearValues[2];
		uint32_t clearValueCount = 0;
		if (vulkanTarget->HasColorAttachment())
		{
			clearValues[0].color.float32[0] = clearValue.color.r;
			clearValues[0].color.float32[1] = clearValue.color.g;
			clearValues[0].color.float32[2] = clearValue.color.b;
			clearValues[0].color.float32[3] = clearValue.color.a;
			if (vulkanTarget->HasDepthStencilAttachment())
			{
				clearValues[1].depthStencil.depth = clearValue.depthStencil.depth;
				clearValues[1].depthStencil.stencil = clearValue.depthStencil.stencil;
				clearValueCount = 2;
			}
			else
			{
				clearValueCount = 1;
			}
		}
		else if (vulkanTarget->HasDepthStencilAttachment())
		{
			clearValues[0].depthStencil.depth = clearValue.depthStencil.depth;
			clearValues[0].depthStencil.stencil = clearValue.depthStencil.stencil;
			clearValueCount = 1;
		}

		renderPassInfo.pClearValues = clearValues;
		renderPassInfo.clearValueCount = clearValueCount;

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

		vkCmdBeginRenderPass(m_CommandBuffer, &renderPassInfo, subpassContents);

		return true;
	}
	return false;
}

bool KVulkanCommandBuffer::ClearColor(const KClearRect& rect, const KClearColor& color)
{
	VkClearValue clearValue;
	VkClearAttachment clearAttachment;

	clearValue.color.float32[0] = color.r;
	clearValue.color.float32[1] = color.g;
	clearValue.color.float32[2] = color.b;
	clearValue.color.float32[3] = color.a;

	clearAttachment.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	clearAttachment.colorAttachment = 0;
	clearAttachment.clearValue = clearValue;

	VkClearRect clearRect;
	clearRect.baseArrayLayer = 0;
	clearRect.layerCount = 1;
	clearRect.rect.offset = { 0 , 0 };
	clearRect.rect.extent = { rect.width, rect.height };

	vkCmdClearAttachments(m_CommandBuffer, 1, &clearAttachment, 1, &clearRect);

	return true;
}

bool KVulkanCommandBuffer::ClearDepthStencil(const KClearRect& rect, const KClearDepthStencil& depthStencil)
{
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
	clearRect.rect.offset = { 0 , 0 };
	clearRect.rect.extent = { rect.width, rect.height };

	vkCmdClearAttachments(m_CommandBuffer, 1, &clearAttachment, 1, &clearRect);

	return true;
}

bool KVulkanCommandBuffer::EndRenderPass()
{
	assert(m_CommandBuffer != VK_NULL_HANDLE);
	if(m_CommandBuffer != VK_NULL_HANDLE)
	{
		vkCmdEndRenderPass(m_CommandBuffer);
		return true;
	}
	return false;
}

bool KVulkanCommandBuffer::End()
{
	assert(m_CommandBuffer != VK_NULL_HANDLE);
	if(m_CommandBuffer != VK_NULL_HANDLE)
	{
		VK_ASSERT_RESULT(vkEndCommandBuffer(m_CommandBuffer));
		return true;
	}
	return false;
}

bool KVulkanCommandBuffer::BeginQuery(IKQueryPtr query)
{
	if (query)
	{
		KVulkanQuery* vulkanQuery = (KVulkanQuery*)query.get();
		vulkanQuery->Begin(m_CommandBuffer);
		return true;
	}
	return false;
}

bool KVulkanCommandBuffer::EndQuery(IKQueryPtr query)
{
	if (query)
	{
		KVulkanQuery* vulkanQuery = (KVulkanQuery*)query.get();
		vulkanQuery->End(m_CommandBuffer);
		return true;
	}
	return false;
}

bool KVulkanCommandBuffer::ResetQuery(IKQueryPtr query)
{
	if (query)
	{
		KVulkanQuery* vulkanQuery = (KVulkanQuery*)query.get();
		vulkanQuery->Reset(m_CommandBuffer);
		return true;
	}
	return false;
}