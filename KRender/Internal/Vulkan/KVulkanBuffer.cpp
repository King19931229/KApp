#include "KVulkanBuffer.h"
#include "KVulkanHelper.h"
#include "KVulkanInitializer.h"
#include "KVulkanGlobal.h"

KVulkanBuffer::KVulkanBuffer()
	: m_BufferSize(0)
	, m_bHostVisible(false)
{
	m_vkBuffer = VK_NULL_HANDLE;
	ZERO_MEMORY(m_AllocInfo);
}

KVulkanBuffer::~KVulkanBuffer()
{
	ASSERT_RESULT(m_ShadowData.empty());
	ASSERT_RESULT(m_vkBuffer == VK_NULL_HANDLE);
}

bool KVulkanBuffer::InitDevice(VkBufferUsageFlags usages, const void* pData, uint32_t bufferSize, bool hostVisible)
{
	ASSERT_RESULT(UnInit());

	using namespace KVulkanGlobal;
	ASSERT_RESULT(m_vkBuffer == VK_NULL_HANDLE);

	if (hostVisible)
	{
		VkBufferUsageFlags usageFlags = usages;

		KVulkanInitializer::CreateVkBuffer(
			(VkDeviceSize)bufferSize
			, usageFlags
			, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
			, m_vkBuffer
			, m_AllocInfo);

		void* data = nullptr;
		VK_ASSERT_RESULT(vkMapMemory(device, m_AllocInfo.vkMemroy, m_AllocInfo.vkOffset, bufferSize, 0, &data));
		memcpy(data, pData, (size_t)bufferSize);
		vkUnmapMemory(device, m_AllocInfo.vkMemroy);
	}
	else
	{
		VkBuffer vkStageBuffer;
		KVulkanHeapAllocator::AllocInfo stageAllocInfo;

		KVulkanInitializer::CreateVkBuffer((VkDeviceSize)bufferSize
			, VK_BUFFER_USAGE_TRANSFER_SRC_BIT
			, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
			, vkStageBuffer
			, stageAllocInfo);

		void* data = nullptr;
		VK_ASSERT_RESULT(vkMapMemory(device, stageAllocInfo.vkMemroy, stageAllocInfo.vkOffset, bufferSize, 0, &data));
		memcpy(data, pData, (size_t)bufferSize);
		vkUnmapMemory(device, stageAllocInfo.vkMemroy);

		VkBufferUsageFlags usageFlags = usages | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

		KVulkanInitializer::CreateVkBuffer((VkDeviceSize)bufferSize
			, usageFlags
			, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
			, m_vkBuffer
			, m_AllocInfo);

		KVulkanInitializer::CopyVkBuffer(vkStageBuffer, m_vkBuffer, (VkDeviceSize)bufferSize);
		KVulkanInitializer::FreeVkBuffer(vkStageBuffer, stageAllocInfo);

		// m_ShadowData = std::move(m_Data);
	}

	m_BufferSize = bufferSize;
	m_bHostVisible = hostVisible;

	return true;
}

bool KVulkanBuffer::UnInit()
{
	using namespace KVulkanGlobal;

	if (m_vkBuffer != VK_NULL_HANDLE)
	{
		KVulkanInitializer::FreeVkBuffer(m_vkBuffer, m_AllocInfo);
		m_vkBuffer = VK_NULL_HANDLE;
		ZERO_MEMORY(m_AllocInfo);
	}

	m_ShadowData.clear();
	m_ShadowData.shrink_to_fit();

	return true;
}

bool KVulkanBuffer::DiscardMemory()
{
	m_ShadowData.clear();
	m_ShadowData.shrink_to_fit();
	return true;
}

bool KVulkanBuffer::Map(void** ppData)
{
	ASSERT_RESULT(ppData != nullptr);
	if (m_bHostVisible)
	{
		if (m_vkBuffer != VK_NULL_HANDLE)
		{
			using namespace KVulkanGlobal;
			VK_ASSERT_RESULT(vkMapMemory(device, m_AllocInfo.vkMemroy, m_AllocInfo.vkOffset, m_BufferSize, 0, ppData));
			return true;
		}
	}
	return false;
}

bool KVulkanBuffer::UnMap()
{
	if (m_bHostVisible)
	{
		if (m_vkBuffer != VK_NULL_HANDLE)
		{
			using namespace KVulkanGlobal;
			vkUnmapMemory(device, m_AllocInfo.vkMemroy);
			return true;
		}
	}
	return false;
}

bool KVulkanBuffer::Write(const void* pData)
{
	if (m_bHostVisible)
	{
		void* pMapData = nullptr;
		if (Map(&pMapData))
		{
			memcpy(pMapData, pData, m_BufferSize);
			UnMap();
			return true;
		}
	}
	return false;
}

bool KVulkanBuffer::Read(void* pData)
{
	if (m_bHostVisible)
	{
		void* pMapData = nullptr;
		if (Map(&pMapData))
		{
			memcpy(pData, pMapData, m_BufferSize);
			UnMap();
			return true;
		}
	}
	else
	{
		if (!m_ShadowData.empty())
		{
			assert(m_ShadowData.size() == m_BufferSize);
			memcpy(pData, m_ShadowData.data(), m_BufferSize);
			return true;
		}
	}
	return false;
}

VkDeviceAddress KVulkanBuffer::GetDeviceAddress() const
{
	VkDeviceAddress address = VK_NULL_HANDEL;
	if (KVulkanHelper::GetBufferDeviceAddress(m_vkBuffer, address))
	{
		return address;
	}
	else
	{
		return VK_NULL_HANDEL;
	}
}

// KVulkanVertexBuffer
KVulkanVertexBuffer::KVulkanVertexBuffer()
	: KVertexBufferBase()
{
}

KVulkanVertexBuffer::~KVulkanVertexBuffer()
{
}

bool KVulkanVertexBuffer::InitDevice(bool hostVisible)
{
	VkBufferUsageFlags usageFlags = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

	if (KVulkanGlobal::supportRaytrace)
	{
		usageFlags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
	}
	if (KVulkanGlobal::supportMeshShader)
	{
		usageFlags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
	}

	return m_Buffer.InitDevice(usageFlags, m_Data.data(), (uint32_t)m_BufferSize, hostVisible);
}

bool KVulkanVertexBuffer::UnInit()
{
	return m_Buffer.UnInit();
}

bool KVulkanVertexBuffer::DiscardMemory()
{
	return m_Buffer.DiscardMemory();
}

bool KVulkanVertexBuffer::Map(void** ppData)
{
	return m_Buffer.Map(ppData);
}

bool KVulkanVertexBuffer::UnMap()
{
	return m_Buffer.UnMap();
}

bool KVulkanVertexBuffer::Write(const void* pData)
{
	return m_Buffer.Write(pData);
}

bool KVulkanVertexBuffer::Read(void* pData)
{
	return m_Buffer.Read(pData);
}

bool KVulkanVertexBuffer::CopyFrom(IKVertexBufferPtr pSource)
{
	return false;
}

bool KVulkanVertexBuffer::CopyTo(IKVertexBufferPtr pDest)
{
	return false;
}

VkBuffer KVulkanVertexBuffer::GetVulkanHandle()
{
	return m_Buffer.GetVulkanHandle();
}

VkDeviceAddress KVulkanVertexBuffer::GetDeviceAddress() const
{
	return m_Buffer.GetDeviceAddress();
}

// KVulkanIndexBuffer
KVulkanIndexBuffer::KVulkanIndexBuffer()
	: KIndexBufferBase()
{
}

KVulkanIndexBuffer::~KVulkanIndexBuffer()
{
}

bool KVulkanIndexBuffer::InitDevice(bool hostVisible)
{
	VkBufferUsageFlags usageFlags = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

	if (KVulkanGlobal::supportRaytrace)
	{
		usageFlags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
	}

	if (KVulkanGlobal::supportMeshShader)
	{
		usageFlags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
	}

	return m_Buffer.InitDevice(usageFlags, m_Data.data(), (uint32_t)m_BufferSize, hostVisible);
}

bool KVulkanIndexBuffer::UnInit()
{
	return m_Buffer.UnInit();
}

bool KVulkanIndexBuffer::DiscardMemory()
{
	return m_Buffer.DiscardMemory();
}

bool KVulkanIndexBuffer::Map(void** ppData)
{
	return m_Buffer.Map(ppData);
}

bool KVulkanIndexBuffer::UnMap()
{
	return m_Buffer.UnMap();
}

bool KVulkanIndexBuffer::Write(const void* pData)
{
	return m_Buffer.Write(pData);
}

bool KVulkanIndexBuffer::Read(void* pData)
{
	return m_Buffer.Read(pData);
}

bool KVulkanIndexBuffer::IsHostVisible() const
{
	return m_Buffer.IsHostVisible();
}

bool KVulkanIndexBuffer::CopyFrom(IKIndexBufferPtr pSource)
{
	return false;
}

bool KVulkanIndexBuffer::CopyTo(IKIndexBufferPtr pDest)
{
	return false;
}

VkBuffer KVulkanIndexBuffer::GetVulkanHandle()
{
	return m_Buffer.GetVulkanHandle();
}

VkDeviceAddress KVulkanIndexBuffer::GetDeviceAddress() const
{
	return m_Buffer.GetDeviceAddress();
}

// KVulkanStorageBuffer
KVulkanStorageBuffer::KVulkanStorageBuffer()
	: KStorageBufferBase()
{
}

KVulkanStorageBuffer::~KVulkanStorageBuffer()
{
}

bool KVulkanStorageBuffer::InitDevice(bool indirect)
{
	KStorageBufferBase::InitDevice(indirect);

	VkBufferUsageFlags usageFlags = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	if (indirect)
	{
		usageFlags |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
	}
	return m_Buffer.InitDevice(usageFlags, m_Data.data(), (uint32_t)m_BufferSize, true);
}

bool KVulkanStorageBuffer::UnInit()
{
	return m_Buffer.UnInit();
}

bool KVulkanStorageBuffer::IsIndirect()
{
	return m_bIndirect;
}

bool KVulkanStorageBuffer::Map(void** ppData)
{
	return m_Buffer.Map(ppData);
}

bool KVulkanStorageBuffer::UnMap()
{
	return m_Buffer.UnMap();
}

bool KVulkanStorageBuffer::Write(const void* pData)
{
	return m_Buffer.Write(pData);
}

bool KVulkanStorageBuffer::Read(void* pData)
{
	return m_Buffer.Read(pData);
}

bool KVulkanStorageBuffer::CopyFrom(IKStorageBufferPtr pSource)
{
	return false;
}

bool KVulkanStorageBuffer::CopyTo(IKStorageBufferPtr pDest)
{
	return false;
}

VkBuffer KVulkanStorageBuffer::GetVulkanHandle()
{
	return m_Buffer.GetVulkanHandle();
}

// KVulkanUniformBuffer
KVulkanUniformBuffer::KVulkanUniformBuffer()
	: KUniformBufferBase()
{
}

KVulkanUniformBuffer::~KVulkanUniformBuffer()
{
}

bool KVulkanUniformBuffer::InitDevice()
{
	VkBufferUsageFlags usageFlags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	return m_Buffer.InitDevice(usageFlags, m_Data.data(), (uint32_t)m_BufferSize, true);
}

bool KVulkanUniformBuffer::UnInit()
{
	return m_Buffer.UnInit();
}

bool KVulkanUniformBuffer::Map(void** ppData)
{
	return m_Buffer.Map(ppData);
}

bool KVulkanUniformBuffer::UnMap()
{
	return m_Buffer.UnMap();
}

bool KVulkanUniformBuffer::Write(const void* pData)
{
	return m_Buffer.Write(pData);
}

bool KVulkanUniformBuffer::Read(void* pData)
{
	return m_Buffer.Read(pData);
}

bool KVulkanUniformBuffer::Reference(void **ppData)
{
	ASSERT_RESULT(ppData != nullptr);
	if(GetVulkanHandle() != VK_NULL_HANDEL)
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

VkBuffer KVulkanUniformBuffer::GetVulkanHandle()
{
	return m_Buffer.GetVulkanHandle();
}