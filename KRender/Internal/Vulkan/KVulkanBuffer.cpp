#include "KVulkanBuffer.h"
#include "KVulkanHelper.h"
#include "KVulkanInitializer.h"
#include "KVulkanGlobal.h"

// KVulkanVertexBuffer
KVulkanVertexBuffer::KVulkanVertexBuffer()
	: KVertexBufferBase(),
	m_bDeviceInit(false)
{
	ZERO_MEMORY(m_vkBuffer);
	ZERO_MEMORY(m_AllocInfo);
}

KVulkanVertexBuffer::~KVulkanVertexBuffer()
{
	ASSERT_RESULT(!m_bDeviceInit);
}

bool KVulkanVertexBuffer::InitDevice()
{
	using namespace KVulkanGlobal;
	ASSERT_RESULT(!m_bDeviceInit);

	VkBuffer vkStageBuffer;
	KVulkanHeapAllocator::AllocInfo stageAllocInfo;

	KVulkanInitializer::CreateVkBuffer((VkDeviceSize)m_BufferSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		vkStageBuffer,
		stageAllocInfo);

	void* data = nullptr;
	VK_ASSERT_RESULT(vkMapMemory(device, stageAllocInfo.vkMemroy, stageAllocInfo.vkOffset, m_BufferSize, 0, &data));
	memcpy(data, m_Data.data(), (size_t) m_BufferSize);
	vkUnmapMemory(device, stageAllocInfo.vkMemroy);

	KVulkanInitializer::CreateVkBuffer((VkDeviceSize)m_BufferSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		m_vkBuffer,
		m_AllocInfo);

	KVulkanHelper::CopyVkBuffer(vkStageBuffer, m_vkBuffer, (VkDeviceSize)m_BufferSize);
	KVulkanInitializer::FreeVkBuffer(vkStageBuffer, stageAllocInfo);

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
		KVulkanInitializer::FreeVkBuffer(m_vkBuffer, m_AllocInfo);
		m_bDeviceInit = false;
	}
	return true;
}

// 暂时先不支持
bool KVulkanVertexBuffer::Write(const void* pData)
{
	return false;
}

bool KVulkanVertexBuffer::Read(void* pData)
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
	: KIndexBufferBase(),
	m_bDeviceInit(false)
{
	ZERO_MEMORY(m_vkBuffer);
	ZERO_MEMORY(m_AllocInfo);
}

KVulkanIndexBuffer::~KVulkanIndexBuffer()
{
	ASSERT_RESULT(!m_bDeviceInit);
}

bool KVulkanIndexBuffer::InitDevice()
{
	using namespace KVulkanGlobal;
	ASSERT_RESULT(!m_bDeviceInit);

	VkBuffer vkStageBuffer;
	KVulkanHeapAllocator::AllocInfo stageAllocInfo;

	KVulkanInitializer::CreateVkBuffer(
		(VkDeviceSize)m_BufferSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		vkStageBuffer,
		stageAllocInfo);

	void* data = nullptr;
	VK_ASSERT_RESULT(vkMapMemory(device, stageAllocInfo.vkMemroy, stageAllocInfo.vkOffset, m_BufferSize, 0, &data));
	memcpy(data, m_Data.data(), (size_t) m_BufferSize);
	vkUnmapMemory(device, stageAllocInfo.vkMemroy);

	KVulkanInitializer::CreateVkBuffer(
		(VkDeviceSize)m_BufferSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		m_vkBuffer,
		m_AllocInfo);

	KVulkanHelper::CopyVkBuffer(vkStageBuffer, m_vkBuffer, (VkDeviceSize)m_BufferSize);
	KVulkanInitializer::FreeVkBuffer(vkStageBuffer, stageAllocInfo);

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
		KVulkanInitializer::FreeVkBuffer(m_vkBuffer, m_AllocInfo);
		m_bDeviceInit = false;
	}
	return true;
}

bool KVulkanIndexBuffer::Write(const void* pData)
{
	return false;
}

bool KVulkanIndexBuffer::Read(void* pData)
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

// KVulkanUniformBuffer
KVulkanUniformBuffer::KVulkanUniformBuffer()
	: KUniformBufferBase(),
	m_bDeviceInit(false)
{
	ZERO_MEMORY(m_AllocInfo);
}

KVulkanUniformBuffer::~KVulkanUniformBuffer()
{
	ASSERT_RESULT(!m_bDeviceInit);
}

bool KVulkanUniformBuffer::InitDevice()
{
	using namespace KVulkanGlobal;
	ASSERT_RESULT(!m_bDeviceInit);

	KVulkanInitializer::CreateVkBuffer((VkDeviceSize)m_BufferSize,
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		m_vkBuffer,
		m_AllocInfo);

	void* data = nullptr;
	VK_ASSERT_RESULT(vkMapMemory(device, m_AllocInfo.vkMemroy, m_AllocInfo.vkOffset, m_BufferSize, 0, &data));
	memcpy(data, m_Data.data(), (size_t) m_BufferSize);
	vkUnmapMemory(device, m_AllocInfo.vkMemroy);

	// 把之前存在内存里的数据丢掉
	m_Data.clear();
	m_bDeviceInit = true;
	return true;
}

bool KVulkanUniformBuffer::UnInit()
{
	using namespace KVulkanGlobal;
	KUniformBufferBase::UnInit();
	if(m_bDeviceInit)
	{
		KVulkanInitializer::FreeVkBuffer(m_vkBuffer, m_AllocInfo);
		m_bDeviceInit = false;
	}
	return true;
}

bool KVulkanUniformBuffer::Write(const void* pData)
{
	using namespace KVulkanGlobal;
	if(m_bDeviceInit && pData)
	{
		void* data = nullptr;
		VK_ASSERT_RESULT(vkMapMemory(device, m_AllocInfo.vkMemroy, m_AllocInfo.vkOffset, m_BufferSize, 0, &data));
		memcpy(data, pData, (size_t) m_BufferSize);
		vkUnmapMemory(device, m_AllocInfo.vkMemroy);
		return true;
	}
	return false;
}

bool KVulkanUniformBuffer::Read(void* pData)
{
	return false;
}

bool KVulkanUniformBuffer::CopyFrom(IKUniformBufferPtr pSource)
{
	return false;
}

bool KVulkanUniformBuffer::CopyTo(IKUniformBufferPtr pDest)
{
	return false;
}