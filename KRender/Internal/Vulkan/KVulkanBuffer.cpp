#include "KVulkanBuffer.h"
#include "KVulkanHelper.h"

KVulkanVertexBuffer::KVulkanVertexBuffer(VkDevice device, VkPhysicalDevice physicalDevice)
	: KVertexBufferBase(),
	m_Device(device),
	m_PhysicalDevice(physicalDevice),
	m_bDeviceInit(false)
{

}

KVulkanVertexBuffer::~KVulkanVertexBuffer()
{

}

bool KVulkanVertexBuffer::InitDevice()
{
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = m_BufferSize;
	bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	if (vkCreateBuffer(m_Device, &bufferInfo, nullptr, &m_vkBuffer) == VK_SUCCESS)
	{
		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(m_Device, m_vkBuffer, &memRequirements);

		VkMemoryAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = UINT32_MAX;

		if(KVulkanHelper::FindMemoryType(
			m_PhysicalDevice,
			memRequirements.memoryTypeBits,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			allocInfo.memoryTypeIndex)
			)
		{
			if (vkAllocateMemory(m_Device, &allocInfo, nullptr, &m_vkDeviceMemory) == VK_SUCCESS)
			{
				vkBindBufferMemory(m_Device, m_vkBuffer, m_vkDeviceMemory, 0);				
				void* data;
				vkMapMemory(m_Device, m_vkDeviceMemory, 0, bufferInfo.size, 0, &data);
				memcpy(data, m_Data.data(), (size_t) bufferInfo.size);
				// 把之前存在内存里的数据丢掉
				m_Data.clear();
				vkUnmapMemory(m_Device, m_vkDeviceMemory);

				m_bDeviceInit = true;
				return true;
			}
		}
		else
		{
			vkDestroyBuffer(m_Device, m_vkBuffer, nullptr);
		}
	}
	m_bDeviceInit = false;
	return false;
}

bool KVulkanVertexBuffer::UnInit()
{
	KVertexBufferBase::UnInit();
	if(m_bDeviceInit)
	{
		vkDestroyBuffer(m_Device, m_vkBuffer, nullptr);
		vkFreeMemory(m_Device, m_vkDeviceMemory, nullptr);
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