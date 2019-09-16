#include "KVulkanBuffer.h"
#include "KVulkanHelper.h"
#include "KVulkanGlobal.h"

bool CreateVkBuffer(VkDeviceSize size,
	VkBufferUsageFlags usage,
	VkMemoryPropertyFlags properties,
	VkBuffer& vkBuffer, VkDeviceMemory& vkBufferMemory)
{
	using namespace KVulkanGlobal;
	if(deviceReady)
	{
		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = usage;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		if (vkCreateBuffer(device, &bufferInfo, nullptr, &vkBuffer) == VK_SUCCESS)
		{
			VkMemoryRequirements memRequirements = {};
			vkGetBufferMemoryRequirements(device, vkBuffer, &memRequirements);

			VkMemoryAllocateInfo allocInfo = {};
			allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			allocInfo.allocationSize = memRequirements.size;
			allocInfo.memoryTypeIndex = UINT32_MAX;

			if(KVulkanHelper::FindMemoryType(
				physicalDevice,
				memRequirements.memoryTypeBits,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				allocInfo.memoryTypeIndex)
				)
			{
				// TODO 使用内存池
				if (vkAllocateMemory(device, &allocInfo, nullptr, &vkBufferMemory) == VK_SUCCESS)
				{
					vkBindBufferMemory(device, vkBuffer, vkBufferMemory, 0);				
					return true;
				}
			}
			else
			{
				vkDestroyBuffer(device, vkBuffer, nullptr);
			}
		}
	}
	return false;
}

bool CopyVkBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
	using namespace KVulkanGlobal;
	if(deviceReady)
	{
		VkCommandBufferAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = graphicsCommandPool;
		allocInfo.commandBufferCount = 1;

		VkCommandBuffer commandBuffer;
		vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);
		{
			VkCommandBufferBeginInfo beginInfo = {};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

			vkBeginCommandBuffer(commandBuffer, &beginInfo);
			{
				VkBufferCopy copyRegion = {};
				copyRegion.srcOffset = 0; // Optional
				copyRegion.dstOffset = 0; // Optional
				copyRegion.size = size;
				vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

				vkEndCommandBuffer(commandBuffer);

				VkSubmitInfo submitInfo = {};
				submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
				submitInfo.commandBufferCount = 1;
				submitInfo.pCommandBuffers = &commandBuffer;

				vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
				vkQueueWaitIdle(graphicsQueue);
			}
		}
		vkFreeCommandBuffers(device, graphicsCommandPool, 1, &commandBuffer);
		return true;
	}
	return false;
}

// KVulkanVertexBuffer
KVulkanVertexBuffer::KVulkanVertexBuffer()
	: KVertexBufferBase(),
	m_bDeviceInit(false)
{

}

KVulkanVertexBuffer::~KVulkanVertexBuffer()
{

}

bool KVulkanVertexBuffer::InitDevice()
{
	using namespace KVulkanGlobal;
	m_bDeviceInit = false;

	VkBuffer vkStageBuffer;
	VkDeviceMemory vkStageBufferMemory;

	if(!CreateVkBuffer((VkDeviceSize)m_BufferSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		vkStageBuffer,
		vkStageBufferMemory))
	{
		return false;
	}

	{
		void* data = nullptr;
		vkMapMemory(device, vkStageBufferMemory, 0, m_BufferSize, 0, &data);
		memcpy(data, m_Data.data(), (size_t) m_BufferSize);
		vkUnmapMemory(device, vkStageBufferMemory);
	}

	if(!CreateVkBuffer((VkDeviceSize)m_BufferSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		m_vkBuffer,
		m_vkDeviceMemory))
	{
		vkDestroyBuffer(device, vkStageBuffer, nullptr);
		vkFreeMemory(device, vkStageBufferMemory, nullptr);
		return false;
	}

	if(!CopyVkBuffer(vkStageBuffer, m_vkBuffer, (VkDeviceSize)m_BufferSize))
	{
		vkDestroyBuffer(device, m_vkBuffer, nullptr);
		vkFreeMemory(device, m_vkDeviceMemory, nullptr);
		vkDestroyBuffer(device, vkStageBuffer, nullptr);
		vkFreeMemory(device, vkStageBufferMemory, nullptr);
		return false;
	}

	vkDestroyBuffer(device, vkStageBuffer, nullptr);
	vkFreeMemory(device, vkStageBufferMemory, nullptr);
	// 把之前存在内存里的数据丢掉
	m_Data.clear();
	m_bDeviceInit = true;
	return true;
}

bool KVulkanVertexBuffer::UnInit()
{
	using namespace KVulkanGlobal;
	KVertexBufferBase::UnInit();
	if(m_bDeviceInit)
	{
		vkDestroyBuffer(device, m_vkBuffer, nullptr);
		vkFreeMemory(device, m_vkDeviceMemory, nullptr);
		m_bDeviceInit = false;
	}
	return true;
}

// 暂时先不支持
bool KVulkanVertexBuffer::Write(const void* pData)
{
	return false;
}

bool KVulkanVertexBuffer::Read(const void* pData)
{
	return false;
}

bool KVulkanVertexBuffer::CopyFrom(IKVertexBufferPtr pSource)
{
	return false;
}

bool KVulkanVertexBuffer::CopyTo(IKVertexBufferPtr pDest)
{
	return false;
}

// KVulkanIndexBuffer
KVulkanIndexBuffer::KVulkanIndexBuffer()
	: KIndexBufferBase()
{

}

KVulkanIndexBuffer::~KVulkanIndexBuffer()
{

}

bool KVulkanIndexBuffer::InitDevice()
{
	using namespace KVulkanGlobal;
	m_bDeviceInit = false;

	VkBuffer vkStageBuffer;
	VkDeviceMemory vkStageBufferMemory;

	if(!CreateVkBuffer((VkDeviceSize)m_BufferSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		vkStageBuffer,
		vkStageBufferMemory))
	{
		return false;
	}

	{
		void* data = nullptr;
		vkMapMemory(device, vkStageBufferMemory, 0, m_BufferSize, 0, &data);
		memcpy(data, m_Data.data(), (size_t) m_BufferSize);
		vkUnmapMemory(device, vkStageBufferMemory);
	}

	if(!CreateVkBuffer((VkDeviceSize)m_BufferSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		m_vkBuffer,
		m_vkDeviceMemory))
	{
		vkDestroyBuffer(device, vkStageBuffer, nullptr);
		vkFreeMemory(device, vkStageBufferMemory, nullptr);
		return false;
	}

	if(!CopyVkBuffer(vkStageBuffer, m_vkBuffer, (VkDeviceSize)m_BufferSize))
	{
		vkDestroyBuffer(device, m_vkBuffer, nullptr);
		vkFreeMemory(device, m_vkDeviceMemory, nullptr);
		vkDestroyBuffer(device, vkStageBuffer, nullptr);
		vkFreeMemory(device, vkStageBufferMemory, nullptr);
		return false;
	}

	vkDestroyBuffer(device, vkStageBuffer, nullptr);
	vkFreeMemory(device, vkStageBufferMemory, nullptr);
	// 把之前存在内存里的数据丢掉
	m_Data.clear();
	m_bDeviceInit = true;
	return true;
}

bool KVulkanIndexBuffer::UnInit()
{
	using namespace KVulkanGlobal;
	KIndexBufferBase::UnInit();
	if(m_bDeviceInit)
	{
		vkDestroyBuffer(device, m_vkBuffer, nullptr);
		vkFreeMemory(device, m_vkDeviceMemory, nullptr);
		m_bDeviceInit = false;
	}
	return true;
}

bool KVulkanIndexBuffer::Write(const void* pData)
{
	return false;
}

bool KVulkanIndexBuffer::Read(const void* pData)
{
	return false;
}

bool KVulkanIndexBuffer::CopyFrom(IKIndexBufferPtr pSource)
{
	return false;
}

bool KVulkanIndexBuffer::CopyTo(IKIndexBufferPtr pDest)
{
	return false;
}