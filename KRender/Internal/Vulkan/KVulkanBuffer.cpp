#include "KVulkanBuffer.h"
#include "KVulkanHelper.h"
#include "KVulkanInitializer.h"
#include "KVulkanGlobal.h"
#include "Internal/KRenderGlobal.h"


KVulkanStageBuffer::KVulkanStageBuffer()
	: m_vkBuffer(VK_NULL_HANDLE)
	, m_BufferSize(0)
	, m_Mapping(false)
{
}

KVulkanStageBuffer::~KVulkanStageBuffer()
{
	ASSERT_RESULT(m_vkBuffer == VK_NULL_HANDLE);
}

bool KVulkanStageBuffer::Init(uint32_t bufferSize)
{
	UnInit();

	m_BufferSize = bufferSize;
	m_Mapping = false;

	KVulkanInitializer::CreateVkBuffer((VkDeviceSize)m_BufferSize
		, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT
		, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
		, m_vkBuffer
		, m_AllocInfo);

	return true;
}

bool KVulkanStageBuffer::UnInit()
{
	KVulkanInitializer::FreeVkBuffer(m_vkBuffer, m_AllocInfo);
	ZERO_MEMORY(m_AllocInfo);
	m_Mapping = false;
	return true;
}

bool KVulkanStageBuffer::Map(void** ppData)
{
	assert(!m_Mapping);
	if (m_vkBuffer != VK_NULL_HANDLE)
	{
		if (m_AllocInfo.pMapped)
		{
			*ppData = m_AllocInfo.pMapped;
		}
		else
		{
			VK_ASSERT_RESULT(vkMapMemory(KVulkanGlobal::device, m_AllocInfo.vkMemroy, m_AllocInfo.vkOffset, m_BufferSize, 0, ppData));
		}
		m_Mapping = true;
		return true;
	}
	return false;
}

bool KVulkanStageBuffer::UnMap()
{
	assert(m_Mapping);
	if (m_vkBuffer != VK_NULL_HANDLE)
	{
		if (!m_AllocInfo.pMapped)
		{
			vkUnmapMemory(KVulkanGlobal::device, m_AllocInfo.vkMemroy);
		}
		m_Mapping = false;
		return true;
	}
	return false;
}

bool KVulkanStageBuffer::Write(const void* pData)
{
	void* pMapData = nullptr;
	if (Map(&pMapData))
	{
		memcpy(pMapData, pData, m_BufferSize);
		UnMap();
		return true;
	}
	return false;
}

bool KVulkanStageBuffer::Read(void* pData)
{
	void* pMapData = nullptr;
	if (Map(&pMapData))
	{
		memcpy(pData, pMapData, m_BufferSize);
		UnMap();
		return true;
	}
	return false;
}

uint32_t KVulkanBuffer::ms_UniqueIDCounter = 0;

KVulkanBuffer::KVulkanBuffer()
	: m_vkBuffer(VK_NULL_HANDLE)
	, m_Usages(0)
	, m_BufferSize(0)
	, m_UniqueID(0)
	, m_bHostVisible(false)
	, m_Mapping(false)
{
	m_vkBuffer = VK_NULL_HANDLE;
	ZERO_MEMORY(m_AllocInfo);
}

KVulkanBuffer::~KVulkanBuffer()
{
	ASSERT_RESULT(m_vkBuffer == VK_NULL_HANDLE);
}

bool KVulkanBuffer::InitDevice(VkBufferUsageFlags usages, const void* pData, uint32_t bufferSize, bool hostVisible)
{
	ASSERT_RESULT(UnInit());
	ASSERT_RESULT(m_vkBuffer == VK_NULL_HANDLE);

	m_Usages = usages;
	m_BufferSize = bufferSize;
	m_bHostVisible = hostVisible;
	m_Mapping = false;

	if (hostVisible)
	{
		VkBufferUsageFlags usageFlags = usages;

		KVulkanInitializer::CreateVkBuffer(
			(VkDeviceSize)m_BufferSize
			, usageFlags
			, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT
			, m_vkBuffer
			, m_AllocInfo);

		if (m_AllocInfo.pMapped)
		{
			memcpy(m_AllocInfo.pMapped, pData, (size_t)m_BufferSize);
		}
		else
		{
			void* data = nullptr;
			VK_ASSERT_RESULT(vkMapMemory(KVulkanGlobal::device, m_AllocInfo.vkMemroy, m_AllocInfo.vkOffset, m_BufferSize, 0, &data));
			memcpy(data, pData, (size_t)m_BufferSize);
			vkUnmapMemory(KVulkanGlobal::device, m_AllocInfo.vkMemroy);
		}
	}
	else
	{
		m_StageBuffer.Init(m_BufferSize);
		m_StageBuffer.Write(pData);

		VkBufferUsageFlags usageFlags = m_Usages | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

		KVulkanInitializer::CreateVkBuffer((VkDeviceSize)m_BufferSize
			, usageFlags
			, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
			, m_vkBuffer
			, m_AllocInfo);
		KVulkanInitializer::CopyVkBuffer(m_StageBuffer.GetVulkanHandle(), m_vkBuffer, (VkDeviceSize)m_BufferSize);
		m_StageBuffer.UnInit();
	}

	m_UniqueID = ++ms_UniqueIDCounter;

	return true;
}

bool KVulkanBuffer::UnInit()
{
	if (m_vkBuffer != VK_NULL_HANDLE)
	{
		KVulkanInitializer::FreeVkBuffer(m_vkBuffer, m_AllocInfo);
		m_vkBuffer = VK_NULL_HANDLE;
		ZERO_MEMORY(m_AllocInfo);
	}
	m_StageBuffer.UnInit();
	m_bHostVisible = false;
	m_Mapping = false;
	return true;
}

bool KVulkanBuffer::SetDebugName(const char* pName)
{
	if (m_vkBuffer != VK_NULL_HANDEL && pName)
	{
		KVulkanHelper::DebugUtilsSetObjectName(KVulkanGlobal::device, (uint64_t)m_vkBuffer, VK_OBJECT_TYPE_BUFFER, pName);
		return true;
	}
	else
	{
		return false;
	}
}

bool KVulkanBuffer::Map(void** ppData)
{
	assert(!m_Mapping);
	ASSERT_RESULT(ppData != nullptr);
	if (m_bHostVisible)
	{
		if (m_vkBuffer != VK_NULL_HANDLE)
		{
			if (m_AllocInfo.pMapped)
			{
				*ppData = m_AllocInfo.pMapped;
			}
			else
			{
				VK_ASSERT_RESULT(vkMapMemory(KVulkanGlobal::device, m_AllocInfo.vkMemroy, m_AllocInfo.vkOffset, m_BufferSize, 0, ppData));
			}
			m_Mapping = true;
			return true;
		}
	}
	else
	{
		m_StageBuffer.Init(m_BufferSize);
		KVulkanInitializer::CopyVkBuffer(m_vkBuffer, m_StageBuffer.GetVulkanHandle(), (VkDeviceSize)m_BufferSize);
		m_Mapping = true;
		if (m_StageBuffer.Map(ppData))
		{
			return true;
		}
	}
	assert(false && "should not reach");
	return false;
}

bool KVulkanBuffer::UnMap()
{
	assert(m_Mapping);
	if (m_bHostVisible)
	{
		if (m_vkBuffer != VK_NULL_HANDLE)
		{
			if (!m_AllocInfo.pMapped)
			{
				vkUnmapMemory(KVulkanGlobal::device, m_AllocInfo.vkMemroy);
			}
			m_Mapping = false;
			return true;
		}
	}
	else
	{
		if (m_StageBuffer.UnMap())
		{
			KVulkanInitializer::CopyVkBuffer(m_StageBuffer.GetVulkanHandle(), m_vkBuffer, (VkDeviceSize)m_BufferSize);
			m_StageBuffer.UnInit();
			m_Mapping = false;
			return true;
		}
	}
	assert(false && "should not reach"); assert(false && "should not reach");
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
	else
	{
		m_StageBuffer.Init(m_BufferSize);
		m_StageBuffer.Write(pData);
		KVulkanInitializer::CopyVkBuffer(m_StageBuffer.GetVulkanHandle(), m_vkBuffer, (VkDeviceSize)m_BufferSize);
		m_StageBuffer.UnInit();
		return true;
	}
	assert(false && "should not reach");
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
		m_StageBuffer.Init(m_BufferSize);
		KVulkanInitializer::CopyVkBuffer(m_vkBuffer, m_StageBuffer.GetVulkanHandle(), (VkDeviceSize)m_BufferSize);
		m_StageBuffer.Read(pData);
		m_StageBuffer.UnInit();
		return true;
	}
	assert(false && "should not reach");
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

	if (KVulkanGlobal::supportNVMeshShader || KVulkanGlobal::supportKHRMeshShader)
	{
		usageFlags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
	}

	return m_Buffer.InitDevice(usageFlags, m_Data.data(), (uint32_t)m_BufferSize, hostVisible);
}

bool KVulkanVertexBuffer::UnInit()
{
	return m_Buffer.UnInit();
}

bool KVulkanVertexBuffer::SetDebugName(const char* pName)
{
	return m_Buffer.SetDebugName(pName);
}

bool KVulkanVertexBuffer::DiscardMemory()
{
	m_Data.clear();
	m_Data.shrink_to_fit();
	return true;
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

	if (KVulkanGlobal::supportNVMeshShader || KVulkanGlobal::supportKHRMeshShader)
	{
		usageFlags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
	}

	return m_Buffer.InitDevice(usageFlags, m_Data.data(), (uint32_t)m_BufferSize, hostVisible);
}

bool KVulkanIndexBuffer::UnInit()
{
	return m_Buffer.UnInit();
}

bool KVulkanIndexBuffer::SetDebugName(const char* pName)
{
	return m_Buffer.SetDebugName(pName);
}

bool KVulkanIndexBuffer::DiscardMemory()
{
	m_Data.clear();
	m_Data.shrink_to_fit();
	return true;
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

bool KVulkanStorageBuffer::InitDevice(bool indirect, bool hostVisible)
{
	KStorageBufferBase::InitDevice(indirect, hostVisible);

	VkBufferUsageFlags usageFlags = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	if (indirect)
	{
		usageFlags |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
	}
	return m_Buffer.InitDevice(usageFlags, m_Data.data(), (uint32_t)m_BufferSize, hostVisible);
}

bool KVulkanStorageBuffer::UnInit()
{
	return m_Buffer.UnInit();
}

bool KVulkanStorageBuffer::SetDebugName(const char* pName)
{
	return m_Buffer.SetDebugName(pName);
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

uint32_t KVulkanStorageBuffer::GetUniqueID() const
{
	return m_Buffer.GetUniqueID();
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
	m_Buffers.resize(KRenderGlobal::NumFramesInFlight);
	for (size_t i = 0; i < m_Buffers.size(); ++i)
	{
		m_Buffers[i].InitDevice(usageFlags, m_Data.data(), (uint32_t)m_BufferSize, true);
	}
	return true;
}

bool KVulkanUniformBuffer::UnInit()
{
	for (size_t i = 0; i < m_Buffers.size(); ++i)
	{
		m_Buffers[i].UnInit();
	}
	return true;
}

bool KVulkanUniformBuffer::SetDebugName(const char* pName)
{
	for (size_t i = 0; i < m_Buffers.size(); ++i)
	{
		m_Buffers[i].SetDebugName((std::string(pName) + "_" + std::to_string(i)).c_str());
	}
	return true;
}

KVulkanBuffer& KVulkanUniformBuffer::GetVulkanBuffer()
{
	ASSERT_RESULT(KRenderGlobal::CurrentInFlightFrameIndex < m_Buffers.size());
	return m_Buffers[KRenderGlobal::CurrentInFlightFrameIndex];
}

bool KVulkanUniformBuffer::Map(void** ppData)
{
	KVulkanBuffer& buffer = GetVulkanBuffer();
	return buffer.Map(ppData);
}

bool KVulkanUniformBuffer::UnMap()
{
	KVulkanBuffer& buffer = GetVulkanBuffer();
	return buffer.UnMap();
}

bool KVulkanUniformBuffer::Write(const void* pData)
{
	KVulkanBuffer& buffer = GetVulkanBuffer();
	return buffer.Write(pData);
}

bool KVulkanUniformBuffer::Read(void* pData)
{
	KVulkanBuffer& buffer = GetVulkanBuffer();
	return buffer.Read(pData);
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
	KVulkanBuffer& buffer = GetVulkanBuffer();
	return buffer.GetVulkanHandle();
}