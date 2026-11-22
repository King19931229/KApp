#include "KVulkanBuffer.h"
#include "KVulkanRenderDevice.h"
#include "KVulkanCommandBuffer.h"
#include "KVulkanHelper.h"
#include "KVulkanInitializer.h"
#include "KVulkanGlobal.h"
#include "Internal/KRenderGlobal.h"
#include "KBase/Publish/KInteger.h"


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

bool KVulkanStageBuffer::Init(uint64_t bufferSize)
{
	if (!KInteger::FitsMemorySize(bufferSize)) return false;
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
	return Write(pData, m_BufferSize);
}

bool KVulkanStageBuffer::Write(const void* pData, uint64_t size)
{
	void* pMapData = nullptr;
	if (size <= m_BufferSize && Map(&pMapData))
	{
		memcpy(pMapData, pData, static_cast<size_t>(size));
		return UnMap();
	}
	return false;
}

bool KVulkanStageBuffer::Read(void* pData)
{
	void* pMapData = nullptr;
	if (Map(&pMapData))
	{
		memcpy(pData, pMapData, static_cast<size_t>(m_BufferSize));
		UnMap();
		return true;
	}
	return false;
}

std::atomic_uint32_t KVulkanBuffer::ms_UniqueIDCounter = 0;

KVulkanBuffer::KVulkanBuffer()
	: m_vkBuffer(VK_NULL_HANDLE)
	, m_Usages(0)
	, m_BufferSize(0)
	, m_UniqueID(0)
	, m_bHostVisible(false)
	, m_Mapping(false)
	, m_MapAccess(MA_READ_WRITE)
{
	m_vkBuffer = VK_NULL_HANDLE;
	ZERO_MEMORY(m_AllocInfo);
}

KVulkanBuffer::~KVulkanBuffer()
{
	ASSERT_RESULT(m_vkBuffer == VK_NULL_HANDLE);
}

bool KVulkanBuffer::InitDevice(VkBufferUsageFlags usages, const void* pData, uint64_t bufferSize, bool hostVisible)
{
	if (!KInteger::FitsMemorySize(bufferSize)) return false;
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

		if (pData && m_AllocInfo.pMapped)
		{
			memcpy(m_AllocInfo.pMapped, pData, (size_t)m_BufferSize);
		}
		else if (pData)
		{
			void* data = nullptr;
			VK_ASSERT_RESULT(vkMapMemory(KVulkanGlobal::device, m_AllocInfo.vkMemroy, m_AllocInfo.vkOffset, m_BufferSize, 0, &data));
			memcpy(data, pData, (size_t)m_BufferSize);
			vkUnmapMemory(KVulkanGlobal::device, m_AllocInfo.vkMemroy);
		}
	}
	else
	{
		if (pData)
		{
			m_StageBuffer.Init(m_BufferSize);
			m_StageBuffer.Write(pData);
		}

		VkBufferUsageFlags usageFlags = m_Usages | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

		KVulkanInitializer::CreateVkBuffer((VkDeviceSize)m_BufferSize
			, usageFlags
			, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
			, m_vkBuffer
			, m_AllocInfo);
		if (pData)
		{
			KVulkanInitializer::CopyVkBuffer(m_StageBuffer.GetVulkanHandle(), m_vkBuffer, (VkDeviceSize)m_BufferSize);
			m_StageBuffer.UnInit();
		}
	}

	if (m_DebugName.c_str())
	{
		KVulkanHelper::DebugUtilsSetObjectName(KVulkanGlobal::device, (uint64_t)m_vkBuffer, VK_OBJECT_TYPE_BUFFER, m_DebugName.c_str());
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
	m_MapAccess = MA_READ_WRITE;
	return true;
}

bool KVulkanBuffer::SetDebugName(const char* pName)
{
	m_DebugName = pName;
	if (m_vkBuffer != VK_NULL_HANDEL && m_DebugName.c_str())
	{
		KVulkanHelper::DebugUtilsSetObjectName(KVulkanGlobal::device, (uint64_t)m_vkBuffer, VK_OBJECT_TYPE_BUFFER, m_DebugName.c_str());
	}
	return true;
}

bool KVulkanBuffer::Map(void** ppData, MapAccess access)
{
	assert(!m_Mapping);
	ASSERT_RESULT(ppData != nullptr);
	if (access != MA_READ_ONLY && access != MA_READ_WRITE && access != MA_WRITE_DISCARD)
	{
		return false;
	}
	if (m_vkBuffer != VK_NULL_HANDLE)
	{
		if (m_bHostVisible)
		{
			if (m_AllocInfo.pMapped)
			{
				*ppData = m_AllocInfo.pMapped;
			}
			else
			{
				VK_ASSERT_RESULT(vkMapMemory(KVulkanGlobal::device, m_AllocInfo.vkMemroy, m_AllocInfo.vkOffset, m_BufferSize, 0, ppData));
			}
			m_MapAccess = access;
			m_Mapping = true;
			return true;
		}
		else
		{
			m_StageBuffer.Init(m_BufferSize);
			if (access != MA_WRITE_DISCARD)
			{
				KVulkanInitializer::CopyVkBuffer(m_vkBuffer, m_StageBuffer.GetVulkanHandle(), (VkDeviceSize)m_BufferSize);
			}
			if (m_StageBuffer.Map(ppData))
			{
				m_MapAccess = access;
				m_Mapping = true;
				return true;
			}
			m_StageBuffer.UnInit();
			return false;
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
			m_MapAccess = MA_READ_WRITE;
			return true;
		}
	}
	else
	{
		if (m_StageBuffer.UnMap())
		{
			if (m_MapAccess != MA_READ_ONLY)
			{
				KVulkanInitializer::CopyVkBuffer(m_StageBuffer.GetVulkanHandle(), m_vkBuffer, (VkDeviceSize)m_BufferSize);
			}
			m_StageBuffer.UnInit();
			m_Mapping = false;
			m_MapAccess = MA_READ_WRITE;
			return true;
		}
	}
	assert(false && "should not reach"); assert(false && "should not reach");
	return false;
}

bool KVulkanBuffer::Write(const void* pData)
{
	return Write(pData, 0, m_BufferSize);
}

bool KVulkanBuffer::RecordUpload(VkCommandBuffer commandBuffer, const void* data, uint64_t size)
{
	if (!data || !size || size > m_BufferSize || m_vkBuffer == VK_NULL_HANDLE || m_bHostVisible) return false;
	if (m_StageBuffer.GetVulkanHandle() == VK_NULL_HANDLE && !m_StageBuffer.Init(m_BufferSize)) return false;
	if (!m_StageBuffer.Write(data, size)) return false;

	VkBufferCopy copy = { 0, 0, size };
	vkCmdCopyBuffer(commandBuffer, m_StageBuffer.GetVulkanHandle(), m_vkBuffer, 1, &copy);
	return true;
}

bool KVulkanBuffer::Write(const void* pData, uint64_t offset, uint64_t size)
{
	if (m_vkBuffer == VK_NULL_HANDLE || m_Mapping || offset > m_BufferSize || size > m_BufferSize - offset || !KInteger::FitsMemorySize(size) || !KInteger::FitsMemorySize(offset) || (size && !pData))
	{
		return false;
	}
	if (size == 0)
	{
		return true;
	}
	if (m_bHostVisible)
	{
		void* pMapData = nullptr;
		if (Map(&pMapData))
		{
			memcpy(POINTER_OFFSET(pMapData, static_cast<size_t>(offset)), pData, static_cast<size_t>(size));
			UnMap();
			return true;
		}
	}
	else
	{
		m_StageBuffer.Init(size);
		const bool success = m_StageBuffer.Write(pData);
		if (success)
		{
			KVulkanInitializer::CopyVkBuffer(m_StageBuffer.GetVulkanHandle(), m_vkBuffer, (VkDeviceSize)size, 0, (VkDeviceSize)offset);
		}
		m_StageBuffer.UnInit();
		return success;
	}
	assert(false && "should not reach");
	return false;
}

bool KVulkanBuffer::Read(void* pData)
{
	return Read(pData, 0, m_BufferSize);
}

bool KVulkanBuffer::Read(void* pData, uint64_t offset, uint64_t size)
{
	if (m_vkBuffer == VK_NULL_HANDLE || m_Mapping || offset > m_BufferSize || size > m_BufferSize - offset || !KInteger::FitsMemorySize(size) || !KInteger::FitsMemorySize(offset) || (size && !pData))
	{
		return false;
	}
	if (size == 0)
	{
		return true;
	}
	if (m_bHostVisible)
	{
		void* pMapData = nullptr;
		if (Map(&pMapData, MA_READ_ONLY))
		{
			memcpy(pData, POINTER_OFFSET(pMapData, static_cast<size_t>(offset)), static_cast<size_t>(size));
			UnMap();
			return true;
		}
	}
	else
	{
		m_StageBuffer.Init(size);
		KVulkanInitializer::CopyVkBuffer(m_vkBuffer, m_StageBuffer.GetVulkanHandle(), (VkDeviceSize)size, (VkDeviceSize)offset, 0);
		const bool success = m_StageBuffer.Read(pData);
		m_StageBuffer.UnInit();
		return success;
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
	VkBufferUsageFlags usageFlags = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

	if (KVulkanGlobal::supportRaytrace)
	{
		usageFlags |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
	}

	if (KVulkanGlobal::supportNVMeshShader || KVulkanGlobal::supportKHRMeshShader)
	{
		usageFlags |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
	}

	return m_Buffer.InitDevice(usageFlags, m_Data.data(), m_BufferSize, hostVisible);
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

bool KVulkanVertexBuffer::Map(void** ppData, MapAccess access)
{
	return m_Buffer.Map(ppData, access);
}

bool KVulkanVertexBuffer::UnMap()
{
	return m_Buffer.UnMap();
}

bool KVulkanVertexBuffer::Write(const void* pData)
{
	return m_Buffer.Write(pData);
}

bool KVulkanVertexBuffer::Write(const void* pData, uint64_t offset, uint64_t size)
{
	return m_Buffer.Write(pData, offset, size);
}

bool KVulkanVertexBuffer::Read(void* pData)
{
	return m_Buffer.Read(pData);
}

bool KVulkanVertexBuffer::Read(void* pData, uint64_t offset, uint64_t size)
{
	return m_Buffer.Read(pData, offset, size);
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

	return m_Buffer.InitDevice(usageFlags, m_Data.data(), m_BufferSize, hostVisible);
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

bool KVulkanIndexBuffer::Map(void** ppData, MapAccess access)
{
	return m_Buffer.Map(ppData, access);
}

bool KVulkanIndexBuffer::UnMap()
{
	return m_Buffer.UnMap();
}

bool KVulkanIndexBuffer::Write(const void* pData)
{
	return m_Buffer.Write(pData);
}

bool KVulkanIndexBuffer::Write(const void* pData, uint64_t offset, uint64_t size)
{
	return m_Buffer.Write(pData, offset, size);
}

bool KVulkanIndexBuffer::Read(void* pData)
{
	return m_Buffer.Read(pData);
}

bool KVulkanIndexBuffer::Read(void* pData, uint64_t offset, uint64_t size)
{
	return m_Buffer.Read(pData, offset, size);
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

// KVulkanBufferBase
KVulkanBufferBase::KVulkanBufferBase(VkBufferUsageFlagBits usage)
	: m_Usage(usage)
{
}

bool KVulkanBufferBase::InitBuffer(bool hostVisible, bool persistent, VkBufferUsageFlags extraUsages)
{
	if (!persistent || m_Ticket)
	{
		UnInitBuffer();
	}
	const uint64_t bufferSize = GetLogicalSize();
	if (!bufferSize) return false;

	const VkBufferUsageFlags usages = m_Usage | extraUsages;
	const auto& initialData = GetInitialData();
	const void* data = initialData.empty() ? nullptr : initialData.data();
	if (persistent)
	{
		m_MappedBuffer = nullptr;
		return m_Buffer.InitDevice(usages, data, bufferSize, hostVisible);
	}

	auto& bufferManager = static_cast<KVulkanRenderDevice*>(KRenderGlobal::RenderDevice)->GetBufferManager();
	m_Ticket = bufferManager.Create(usages, data, bufferSize, hostVisible);
	if (m_Ticket && !m_Buffer.GetDebugName().empty())
	{
		bufferManager.SetDebugName(m_Ticket, m_Buffer.GetDebugName().c_str());
	}
	return m_Ticket != nullptr;
}

bool KVulkanBufferBase::UnInitBuffer()
{
	if (m_Ticket)
	{
		static_cast<KVulkanRenderDevice*>(KRenderGlobal::RenderDevice)->GetBufferManager().Destroy(m_Ticket);
		m_Ticket = nullptr;
	}
	m_MappedBuffer = nullptr;
	return m_Buffer.UnInit();
}

bool KVulkanBufferBase::SetDebugName(const char* pName)
{
	if (m_Ticket)
	{
		static_cast<KVulkanRenderDevice*>(KRenderGlobal::RenderDevice)->GetBufferManager().SetDebugName(m_Ticket, pName);
	}
	return m_Buffer.SetDebugName(pName);
}

KVulkanBuffer* KVulkanBufferBase::GetMemoryBuffer(bool readback)
{
	if (!m_Ticket) return &GetFixedBuffer();
	if (!m_Ticket->bufferSize) return nullptr;
	// Read the last storage allocation without borrowing for the current frame.
	if (readback && m_Usage == VK_BUFFER_USAGE_STORAGE_BUFFER_BIT) return m_Ticket->current.buffer;

	const auto& initialData = GetInitialData();
	const void* data = initialData.empty() ? nullptr : initialData.data();
	return &static_cast<KVulkanRenderDevice*>(KRenderGlobal::RenderDevice)->GetBufferManager().Request(m_Ticket, data);
}

KVulkanBuffer* KVulkanBufferBase::GetVulkanBuffer()
{
	return m_Ticket && !m_Ticket->bufferSize ? m_Ticket->current.buffer : GetMemoryBuffer();
}

bool KVulkanBufferBase::Map(void** ppData, MapAccess access)
{
	ASSERT_RESULT(!m_MappedBuffer);
	KVulkanBuffer* buffer = GetMemoryBuffer();
	if (!buffer || !buffer->Map(ppData, access)) return false;
	m_MappedBuffer = buffer;
	return true;
}

bool KVulkanBufferBase::UnMap()
{
	ASSERT_RESULT(m_MappedBuffer);
	if (!m_MappedBuffer->UnMap()) return false;
	m_MappedBuffer = nullptr;
	return true;
}

bool KVulkanBufferBase::Write(const void* pData)
{
	KVulkanBuffer* buffer = GetMemoryBuffer();
	return buffer && buffer->Write(pData);
}

bool KVulkanBufferBase::Write(const void* pData, uint64_t offset, uint64_t size)
{
	KVulkanBuffer* buffer = GetMemoryBuffer();
	return buffer && buffer->Write(pData, offset, size);
}

bool KVulkanBufferBase::Read(void* pData)
{
	KVulkanBuffer* buffer = GetMemoryBuffer(true);
	return buffer && buffer->Read(pData);
}

bool KVulkanBufferBase::Read(void* pData, uint64_t offset, uint64_t size)
{
	KVulkanBuffer* buffer = GetMemoryBuffer(true);
	return buffer && buffer->Read(pData, offset, size);
}

bool KVulkanBufferBase::RecordUpload(KVulkanCommandBuffer& commandBuffer, const void* data, uint64_t offset, uint64_t size)
{
	if (m_Ticket)
	{
		if (m_Ticket->bufferSize) return false;
		m_Ticket->current = {};
	}

	const auto& limits = KVulkanGlobal::deviceProperties.limits;
	const uint64_t maxRange = m_Usage == VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT ? limits.maxUniformBufferRange : limits.maxStorageBufferRange;
	VkCommandBuffer cmdBuf = commandBuffer.GetVkHandle();
	if (cmdBuf == VK_NULL_HANDLE || commandBuffer.GetVkBufferLevel() != VK_COMMAND_BUFFER_LEVEL_PRIMARY ||
		offset || !data || !size || size > maxRange || GetLogicalSize())
	{
		return false;
	}

	KVulkanBufferManager& bufferManager = static_cast<KVulkanRenderDevice*>(KRenderGlobal::RenderDevice)->GetBufferManager();
	if (!m_Ticket)
	{
		m_Ticket = bufferManager.Create(m_Usage, false);
		bufferManager.SetDebugName(m_Ticket, m_Buffer.GetDebugName().c_str());
	}

	KVulkanBuffer* buffer = bufferManager.RequestUpload(m_Ticket, size, commandBuffer.GetFrameNum());
	if (!buffer || !buffer->RecordUpload(cmdBuf, data, size))
	{
		return false;
	}

	const VkAccessFlags access = m_Usage == VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT ? VK_ACCESS_UNIFORM_READ_BIT : VK_ACCESS_SHADER_READ_BIT;
	KVulkanInitializer::TransitionBufferCmdBuffer(buffer->GetVulkanHandle(), size,
		VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		VK_ACCESS_TRANSFER_WRITE_BIT, access, cmdBuf);
	m_Ticket->current = { buffer, size };
	return true;
}

uint64_t KVulkanBufferBase::GetBufferSize()
{
	return m_Ticket && !m_Ticket->bufferSize ? m_Ticket->current.size : GetLogicalSize();
}

VkBuffer KVulkanBufferBase::GetVulkanHandle()
{
	KVulkanBuffer* buffer = GetVulkanBuffer();
	return buffer ? buffer->GetVulkanHandle() : VK_NULL_HANDLE;
}

uint32_t KVulkanBufferBase::GetUniqueID() const
{
	KVulkanBuffer* buffer = const_cast<KVulkanBufferBase*>(this)->GetVulkanBuffer();
	return buffer ? buffer->GetUniqueID() : 0;
}

// KVulkanStorageBuffer
KVulkanStorageBuffer::KVulkanStorageBuffer()
	: KVulkanBufferBase(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT)
{
}

bool KVulkanStorageBuffer::InitFromVertexBuffer(IKVertexBufferPtr pSource)
{
	if (!pSource || !pSource->GetBufferSize())
	{
		return false;
	}
	KVulkanBuffer& buffer = static_cast<KVulkanVertexBuffer*>(pSource.get())->m_Buffer;
	if (buffer.GetVulkanHandle() == VK_NULL_HANDLE)
	{
		return false;
	}

	UnInit();
	// 与源顶点缓冲共享所有权。
	m_ReferencedBuffer = std::shared_ptr<KVulkanBuffer>(pSource, &buffer);
	m_BufferSize = pSource->GetBufferSize();
	m_bIndirect = false;
	return true;
}

bool KVulkanStorageBuffer::InitDevice(bool indirect, bool hostVisible, bool persistent)
{
	m_ReferencedBuffer.reset();
	KStorageBufferBase::InitDevice(indirect, hostVisible, persistent);
	if (!InitBuffer(hostVisible, persistent, indirect ? VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT : 0)) return false;
	m_Data.clear();
	m_Data.shrink_to_fit();
	return true;
}

bool KVulkanStorageBuffer::UnInit()
{
	m_ReferencedBuffer.reset();
	KStorageBufferBase::UnInit();
	m_Data.shrink_to_fit();
	return UnInitBuffer();
}

KVulkanBuffer& KVulkanStorageBuffer::GetFixedBuffer()
{
	return m_ReferencedBuffer ? *m_ReferencedBuffer : m_Buffer;
}

bool KVulkanStorageBuffer::CopyFrom(IKStorageBufferPtr pSource)
{
	return false;
}

bool KVulkanStorageBuffer::CopyTo(IKStorageBufferPtr pDest)
{
	return false;
}

// KVulkanUniformBuffer
KVulkanUniformBuffer::KVulkanUniformBuffer()
	: KVulkanBufferBase(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT)
{
}

bool KVulkanUniformBuffer::InitDevice(bool persistent)
{
	return InitBuffer(true, persistent);
}

bool KVulkanUniformBuffer::UnInit()
{
	return UnInitBuffer();
}

bool KVulkanUniformBuffer::Reference(void **ppData)
{
	ASSERT_RESULT(ppData != nullptr);
	if (m_BufferSize && GetVulkanHandle() != VK_NULL_HANDLE)
	{
		ASSERT_RESULT(m_Data.size() == m_BufferSize);
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
