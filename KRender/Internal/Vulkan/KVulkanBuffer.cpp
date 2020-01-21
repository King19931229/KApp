#include "KVulkanBuffer.h"
#include "KVulkanHelper.h"
#include "KVulkanInitializer.h"
#include "KVulkanGlobal.h"

// KVulkanVertexBuffer
KVulkanVertexBuffer::KVulkanVertexBuffer()
	: KVertexBufferBase(),
	m_bDeviceInit(false),
	m_bHostVisible(false)
{
	m_vkBuffer = VK_NULL_HANDLE;
	ZERO_MEMORY(m_AllocInfo);
}

KVulkanVertexBuffer::~KVulkanVertexBuffer()
{
	ASSERT_RESULT(!m_bDeviceInit);
	ASSERT_RESULT(m_vkBuffer == VK_NULL_HANDLE);
}

bool KVulkanVertexBuffer::InitDevice(bool hostVisible)
{
	using namespace KVulkanGlobal;
	ASSERT_RESULT(!m_bDeviceInit);

	if(hostVisible)
	{
		KVulkanInitializer::CreateVkBuffer(
			(VkDeviceSize)m_BufferSize,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			m_vkBuffer,
			m_AllocInfo);

		void* data = nullptr;
		VK_ASSERT_RESULT(vkMapMemory(device, m_AllocInfo.vkMemroy, m_AllocInfo.vkOffset, m_BufferSize, 0, &data));
		memcpy(data, m_Data.data(), (size_t) m_BufferSize);
		vkUnmapMemory(device, m_AllocInfo.vkMemroy);
	}
	else
	{
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
	}

	// 把之前存在内存里的数据丢掉
	if(hostVisible)
	{
		m_Data.swap(decltype(m_Data)());
	}
	m_bDeviceInit = true;
	m_bHostVisible = hostVisible;

	return true;
}

bool KVulkanVertexBuffer::UnInit()
{
	using namespace KVulkanGlobal;
	KVertexBufferBase::UnInit();
	if(m_bDeviceInit)
	{
		KVulkanInitializer::FreeVkBuffer(m_vkBuffer, m_AllocInfo);
		m_vkBuffer = VK_NULL_HANDLE;
		ZERO_MEMORY(m_AllocInfo);
		m_bDeviceInit = false;
	}
	return true;
}

bool KVulkanVertexBuffer::Map(void** ppData)
{
	ASSERT_RESULT(ppData != nullptr);
	if(m_bHostVisible)
	{
		if(m_bDeviceInit)
		{
			using namespace KVulkanGlobal;
			VK_ASSERT_RESULT(vkMapMemory(device, m_AllocInfo.vkMemroy, m_AllocInfo.vkOffset, m_BufferSize, 0, ppData));
			return true;
		}
	}
	return false;
}

bool KVulkanVertexBuffer::UnMap()
{
	if(m_bDeviceInit && m_bHostVisible)
	{
		using namespace KVulkanGlobal;
		vkUnmapMemory(device, m_AllocInfo.vkMemroy);
		return true;
	}
	return false;
}

bool KVulkanVertexBuffer::Write(const void* pData)
{
	if(m_bHostVisible)
	{
		void* pMapData = nullptr;
		if(Map(&pMapData))
		{
			memcpy(pMapData, pData, m_BufferSize);
			UnMap();
			return true;
		}
	}
	return false;
}

bool KVulkanVertexBuffer::Read(void* pData)
{
	if(m_bHostVisible)
	{
		void* pMapData = nullptr;
		if(Map(&pMapData))
		{
			memcpy(pData, pMapData, m_BufferSize);
			UnMap();
			return true;
		}
	}
	else
	{
		if(!m_Data.empty())
		{
			assert(m_Data.size() == m_BufferSize);
			memcpy(pData, m_Data.data(), m_BufferSize);
			return true;
		}
	}
	return true;
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
	m_bDeviceInit(false),
	m_bHostVisible(false)
{
	m_vkBuffer = VK_NULL_HANDLE;
	ZERO_MEMORY(m_AllocInfo);
}

KVulkanIndexBuffer::~KVulkanIndexBuffer()
{
	ASSERT_RESULT(!m_bDeviceInit);
	ASSERT_RESULT(m_vkBuffer == VK_NULL_HANDLE);
}

bool KVulkanIndexBuffer::InitDevice(bool hostVisible)
{
	using namespace KVulkanGlobal;
	ASSERT_RESULT(!m_bDeviceInit);

	if(hostVisible)
	{
		KVulkanInitializer::CreateVkBuffer(
			(VkDeviceSize)m_BufferSize,
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			m_vkBuffer,
			m_AllocInfo);

		void* data = nullptr;
		VK_ASSERT_RESULT(vkMapMemory(device, m_AllocInfo.vkMemroy, m_AllocInfo.vkOffset, m_BufferSize, 0, &data));
		memcpy(data, m_Data.data(), (size_t) m_BufferSize);
		vkUnmapMemory(device, m_AllocInfo.vkMemroy);
	}
	else
	{
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
	}

	// 把之前存在内存里的数据丢掉
	if(hostVisible)
	{
		m_Data.swap(decltype(m_Data)());
	}
	m_bDeviceInit = true;
	m_bHostVisible = hostVisible;

	return true;
}

bool KVulkanIndexBuffer::UnInit()
{
	using namespace KVulkanGlobal;
	KIndexBufferBase::UnInit();
	if(m_bDeviceInit)
	{
		KVulkanInitializer::FreeVkBuffer(m_vkBuffer, m_AllocInfo);
		m_vkBuffer = VK_NULL_HANDLE;
		ZERO_MEMORY(m_AllocInfo);
		m_bDeviceInit = false;
	}
	return true;
}

bool KVulkanIndexBuffer::Map(void** ppData)
{
	ASSERT_RESULT(ppData != nullptr);
	if(m_bHostVisible)
	{
		if(m_bDeviceInit)
		{
			using namespace KVulkanGlobal;
			VK_ASSERT_RESULT(vkMapMemory(device, m_AllocInfo.vkMemroy, m_AllocInfo.vkOffset, m_BufferSize, 0, ppData));
			return true;
		}
	}
	return false;
}

bool KVulkanIndexBuffer::UnMap()
{
	if(m_bDeviceInit && m_bHostVisible)
	{
		using namespace KVulkanGlobal;
		vkUnmapMemory(device, m_AllocInfo.vkMemroy);
		return true;
	}
	return false;
}

bool KVulkanIndexBuffer::Write(const void* pData)
{
	if(m_bHostVisible)
	{
	void* pMapData = nullptr;
	if(Map(&pMapData))
	{
		memcpy(pMapData, pData, m_BufferSize);
		UnMap();
		return true;
	}
	}
	return false;
}

bool KVulkanIndexBuffer::Read(void* pData)
{
	if(m_bHostVisible)
	{
		void* pMapData = nullptr;
		if(Map(&pMapData))
		{
			memcpy(pData, pMapData, m_BufferSize);
			UnMap();
			return true;
		}
	}
	else
	{
		if(!m_Data.empty())
		{
			assert(m_Data.size() == m_BufferSize);
			memcpy(pData, m_Data.data(), m_BufferSize);
			return true;
		}
	}
	return true;
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
	m_vkBuffer = VK_NULL_HANDLE;
	ZERO_MEMORY(m_AllocInfo);
}

KVulkanUniformBuffer::~KVulkanUniformBuffer()
{
	ASSERT_RESULT(!m_bDeviceInit);
	ASSERT_RESULT(m_vkBuffer == VK_NULL_HANDLE);
}

bool KVulkanUniformBuffer::InitDevice()
{
	using namespace KVulkanGlobal;
	ASSERT_RESULT(!m_bDeviceInit);

	KVulkanInitializer::CreateVkBuffer((VkDeviceSize)m_BufferSize,
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		m_vkBuffer,
		m_AllocInfo);

	void* data = nullptr;
	VK_ASSERT_RESULT(vkMapMemory(device, m_AllocInfo.vkMemroy, m_AllocInfo.vkOffset, m_BufferSize, 0, &data));
	memcpy(data, m_Data.data(), (size_t) m_BufferSize);
	vkUnmapMemory(device, m_AllocInfo.vkMemroy);

	// 把之前存在内存里的数据丢掉
	m_Data.swap(decltype(m_Data)());
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
		m_vkBuffer = VK_NULL_HANDLE;
		ZERO_MEMORY(m_AllocInfo);

		m_bDeviceInit = false;
	}
	return true;
}

bool KVulkanUniformBuffer::Write(const void* pData)
{
	ASSERT_RESULT(KVulkanGlobal::deviceReady);
	ASSERT_RESULT(m_bDeviceInit);
	ASSERT_RESULT(pData != nullptr);

	void* data = nullptr;
	VK_ASSERT_RESULT(vkMapMemory(KVulkanGlobal::device, m_AllocInfo.vkMemroy, m_AllocInfo.vkOffset, m_BufferSize, 0, &data));
	memcpy(data, pData, (size_t) m_BufferSize);
	vkUnmapMemory(KVulkanGlobal::device, m_AllocInfo.vkMemroy);

	return true;
}

bool KVulkanUniformBuffer::Read(void* pData)
{
	if(m_bDeviceInit)
	{
		ASSERT_RESULT(m_Data.size() == m_BufferSize);
		ASSERT_RESULT(m_BufferSize > 0);
		memcpy(pData, m_Data.data(), m_BufferSize);
		return true;
	}
	return false;
}

bool KVulkanUniformBuffer::Reference(void **ppData)
{
	ASSERT_RESULT(ppData != nullptr);
	if(m_bDeviceInit)
	{
		ASSERT_RESULT(m_Data.size() == m_BufferSize);
		ASSERT_RESULT(m_BufferSize > 0);
		*ppData = m_Data.data();
		return true;
	}
	else
	{
		*ppData = nullptr;
		return false;
	}
}

bool KVulkanUniformBuffer::CopyFrom(IKUniformBufferPtr pSource)
{
	return false;
}

bool KVulkanUniformBuffer::CopyTo(IKUniformBufferPtr pDest)
{
	return false;
}