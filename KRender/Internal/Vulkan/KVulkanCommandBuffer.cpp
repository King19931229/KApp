#include "KVulkanCommandBuffer.h"
#include "KVulkanRenderTarget.h"
#include "KVulkanPipeline.h"
#include "KVulkanBuffer.h"
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
	m_CommandLevel(VK_COMMAND_BUFFER_LEVEL_MAX_ENUM),
	m_SubpassContents(VK_SUBPASS_CONTENTS_MAX_ENUM)
{

}

KVulkanCommandBuffer::~KVulkanCommandBuffer()
{

}

bool KVulkanCommandBuffer::Init(IKCommandPool* pool, CommandBufferLevel level)
{
	ASSERT_RESULT(UnInit());

	ASSERT_RESULT(KVulkanGlobal::deviceReady);
	KVulkanCommandPool* vulkanPool = (KVulkanCommandPool*)pool;

	m_ParentPool = vulkanPool->GetVkHandle();
	ASSERT_RESULT(m_ParentPool != VK_NULL_HANDLE);

	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = m_ParentPool;

	switch (level)
	{
	case CBL_PRIMARY:
		m_CommandLevel = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		m_SubpassContents = VK_SUBPASS_CONTENTS_INLINE;
		break;
	case CBL_SECONDARY:
		m_CommandLevel = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
		m_SubpassContents = VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS;
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
	m_SubpassContents = VK_SUBPASS_CONTENTS_MAX_ENUM;

	return true;
}

bool KVulkanCommandBuffer::SetViewport(IKRenderTarget* target)
{
	assert(m_CommandBuffer != VK_NULL_HANDLE);
	if(m_CommandBuffer != VK_NULL_HANDLE)
	{
		KVulkanRenderTarget* vulkanTarget = (KVulkanRenderTarget*)target;

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
			assert(false && "render command not complete. check the logic");
			return false;
		}

		if(!command.pipelineHandle)
		{
			return false;
		}

		KVulkanPipeline* vulkanPipeline = (KVulkanPipeline*)command.pipeline;
		KVulkanPipelineHandle* pipelineHandle = (KVulkanPipelineHandle*)command.pipelineHandle;

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
			vertexBuffers[i] = ((KVulkanVertexBuffer*)vertexBuffer.get())->GetVulkanHandle();
			offsets[i] = 0;
		}

		vkCmdBindVertexBuffers(m_CommandBuffer, 0, vertexBufferCount, vertexBuffers, offsets);

		if(command.useObjectData)
		{
			KConstantDefinition::OBJECT* objectData  = (KConstantDefinition::OBJECT*)command.objectData;
			vkCmdPushConstants(m_CommandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, command.objectPushOffset, sizeof(KConstantDefinition::OBJECT), objectData);
		}

		if(command.indexDraw)
		{
			// 绑定索引缓冲
			KVulkanIndexBuffer* vulkanIndexBuffer = ((KVulkanIndexBuffer*)command.indexData->indexBuffer.get());
			vkCmdBindIndexBuffer(m_CommandBuffer, vulkanIndexBuffer->GetVulkanHandle(), 0, vulkanIndexBuffer->GetVulkanIndexType());
			vkCmdDrawIndexed(m_CommandBuffer, command.indexData->indexCount, 1, command.indexData->indexStart, 0, 0);
		}
		else
		{
			vkCmdDraw(m_CommandBuffer, command.vertexData->vertexCount, 1, command.vertexData->vertexStart, 0);
		}

		return true;
	}
	return false;
}

bool KVulkanCommandBuffer::Execute(KCommandBufferList& commandBuffers)
{
	assert(m_CommandBuffer != VK_NULL_HANDLE);
	assert(m_CommandLevel == VK_COMMAND_BUFFER_LEVEL_PRIMARY);
	assert(commandBuffers.size() > 0);

	if(m_CommandBuffer != VK_NULL_HANDLE && m_CommandLevel == VK_COMMAND_BUFFER_LEVEL_PRIMARY && commandBuffers.size() > 0)
	{
		std::vector<VkCommandBuffer> vkCommandBuffers;

		for(IKCommandBuffer* buffer : commandBuffers)
		{
			VkCommandBuffer handle = ((KVulkanCommandBuffer*)buffer)->GetVkHandle();
			VkCommandBufferLevel level = ((KVulkanCommandBuffer*)buffer)->GetVkBufferLevel();
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

bool KVulkanCommandBuffer::BeginSecondary(IKRenderTarget* target)
{
	assert(m_CommandBuffer != VK_NULL_HANDLE);
	assert(m_CommandLevel == VK_COMMAND_BUFFER_LEVEL_SECONDARY);

	if(m_CommandBuffer != VK_NULL_HANDLE && m_CommandLevel == VK_COMMAND_BUFFER_LEVEL_SECONDARY)
	{
		KVulkanRenderTarget* vulkanTarget = (KVulkanRenderTarget*)target;

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

bool KVulkanCommandBuffer::BeginRenderPass(IKRenderTarget* target)
{
	assert(m_CommandBuffer != VK_NULL_HANDLE);
	if(m_CommandBuffer != VK_NULL_HANDLE)
	{
		KVulkanRenderTarget* vulkanTarget = (KVulkanRenderTarget*)target;

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
		auto clearValuesPair = vulkanTarget->GetVkClearValues();
		renderPassInfo.pClearValues = clearValuesPair.first;
		renderPassInfo.clearValueCount = clearValuesPair.second;
		vkCmdBeginRenderPass(m_CommandBuffer, &renderPassInfo, m_SubpassContents);

		return true;
	}
	return false;
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