#pragma once
#include "Internal/KBufferBase.h"
#include "KVulkanConfig.h"
#include "KVulkanHeapAllocator.h"

class KVulkanBuffer
{
protected:
	VkBuffer m_vkBuffer;
	KVulkanHeapAllocator::AllocInfo m_AllocInfo;
	std::vector<char> m_ShadowData;
	uint32_t m_BufferSize;
	bool m_bHostVisible;
public:
	KVulkanBuffer();
	~KVulkanBuffer();

	bool InitDevice(VkBufferUsageFlags usages, const void* pData, uint32_t bufferSize, bool hostVisible);
	bool UnInit();

	bool DiscardMemory();

	bool Map(void** ppData);
	bool UnMap();
	bool Write(const void* pData);
	bool Read(void* pData);

	inline bool IsHostVisible() const { return m_bHostVisible; }
	inline VkBuffer GetVulkanHandle() { return m_vkBuffer; }
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

	virtual bool IsIndirect();

	virtual bool Map(void** ppData);
	virtual bool UnMap();
	virtual bool Write(const void* pData);
	virtual bool Read(void* pData);

	virtual bool CopyFrom(IKStorageBufferPtr pSource);
	virtual bool CopyTo(IKStorageBufferPtr pDest);

	VkBuffer GetVulkanHandle();
};

class KVulkanUniformBuffer : public KUniformBufferBase
{
protected:
	KVulkanBuffer m_Buffer;
public:
	KVulkanUniformBuffer();
	virtual ~KVulkanUniformBuffer();

	virtual bool InitDevice();
	virtual bool UnInit();

	virtual bool Map(void** ppData);
	virtual bool UnMap();
	virtual bool Write(const void* pData);
	virtual bool Read(void* pData);

	virtual bool Reference(void **ppData);

	virtual bool CopyFrom(IKUniformBufferPtr pSource);
	virtual bool CopyTo(IKUniformBufferPtr pDest);

	VkBuffer GetVulkanHandle();
};