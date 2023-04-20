#pragma once
#include "Internal/KBufferBase.h"
#include "KVulkanConfig.h"
#include "KVulkanHeapAllocator.h"

class KVulkanStageBuffer
{
protected:
	VkBuffer m_vkBuffer;
	KVulkanHeapAllocator::AllocInfo m_AllocInfo;
	uint32_t m_BufferSize;
	bool m_Mapping;
public:
	KVulkanStageBuffer();
	~KVulkanStageBuffer();

	bool Init(uint32_t bufferSize);
	bool UnInit();

	bool Map(void** ppData);
	bool UnMap();
	bool Write(const void* pData);
	bool Read(void* pData);

	inline VkBuffer GetVulkanHandle() { return m_vkBuffer; }
};

class KVulkanBuffer
{
protected:
	VkBuffer m_vkBuffer;
	KVulkanHeapAllocator::AllocInfo m_AllocInfo;
	VkBufferUsageFlags m_Usages;
	KVulkanStageBuffer m_StageBuffer;
	uint32_t m_BufferSize;
	uint32_t m_UniqueID;
	bool m_bHostVisible;
	bool m_Mapping;

	static uint32_t ms_UniqueIDCounter;
public:
	KVulkanBuffer();
	~KVulkanBuffer();

	bool InitDevice(VkBufferUsageFlags usages, const void* pData, uint32_t bufferSize, bool hostVisible);
	bool UnInit();

	bool SetDebugName(const char* pName);

	bool Map(void** ppData);
	bool UnMap();
	bool Write(const void* pData);
	bool Read(void* pData);

	inline bool IsHostVisible() const { return m_bHostVisible; }
	inline VkBuffer GetVulkanHandle() { return m_vkBuffer; }
	inline uint32_t GetUniqueID() const { return m_UniqueID; }

	VkDeviceAddress GetDeviceAddress() const;
};

class KVulkanVertexBuffer : public KVertexBufferBase
{
protected:
	KVulkanBuffer m_Buffer;
public:
	KVulkanVertexBuffer();
	~KVulkanVertexBuffer();

	virtual bool InitDevice(bool hostVisible);
	virtual bool UnInit();

	virtual bool SetDebugName(const char* pName);

	virtual bool DiscardMemory();

	virtual bool Map(void** ppData);
	virtual bool UnMap();
	virtual bool Write(const void* pData);
	virtual bool Read(void* pData);

	virtual bool CopyFrom(IKVertexBufferPtr pSource);
	virtual bool CopyTo(IKVertexBufferPtr pDest);

	VkBuffer GetVulkanHandle();
	VkDeviceAddress GetDeviceAddress() const;
};

class KVulkanIndexBuffer : public KIndexBufferBase
{
protected:
	KVulkanBuffer m_Buffer;
public:
	KVulkanIndexBuffer();
	virtual ~KVulkanIndexBuffer();

	virtual bool InitDevice(bool hostVisible);
	virtual bool UnInit();

	virtual bool SetDebugName(const char* pName);

	virtual bool DiscardMemory();

	virtual bool Map(void** ppData);
	virtual bool UnMap();
	virtual bool Write(const void* pData);
	virtual bool Read(void* pData);

	virtual bool IsHostVisible() const;

	virtual bool CopyFrom(IKIndexBufferPtr pSource);
	virtual bool CopyTo(IKIndexBufferPtr pDest);

	inline VkIndexType GetVulkanIndexType() { return m_IndexType == IT_16 ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32; }

	VkBuffer GetVulkanHandle();
	VkDeviceAddress GetDeviceAddress() const;
};

class KVulkanStorageBuffer : public KStorageBufferBase
{
protected:
	KVulkanBuffer m_Buffer;
public:
	KVulkanStorageBuffer();
	virtual ~KVulkanStorageBuffer();

	virtual bool InitDevice(bool indirect);
	virtual bool UnInit();

	virtual bool SetDebugName(const char* pName);

	virtual bool IsIndirect();

	virtual bool Map(void** ppData);
	virtual bool UnMap();
	virtual bool Write(const void* pData);
	virtual bool Read(void* pData);

	virtual bool CopyFrom(IKStorageBufferPtr pSource);
	virtual bool CopyTo(IKStorageBufferPtr pDest);

	VkBuffer GetVulkanHandle();
	uint32_t GetUniqueID() const;
};

class KVulkanUniformBuffer : public KUniformBufferBase
{
protected:
	std::vector<KVulkanBuffer> m_Buffers;
	KVulkanBuffer& GetVulkanBuffer();
public:
	KVulkanUniformBuffer();
	virtual ~KVulkanUniformBuffer();

	virtual bool InitDevice();
	virtual bool UnInit();

	virtual bool SetDebugName(const char* pName);

	virtual bool Map(void** ppData);
	virtual bool UnMap();
	virtual bool Write(const void* pData);
	virtual bool Read(void* pData);

	virtual bool Reference(void **ppData);

	virtual bool CopyFrom(IKUniformBufferPtr pSource);
	virtual bool CopyTo(IKUniformBufferPtr pDest);

	VkBuffer GetVulkanHandle();
};