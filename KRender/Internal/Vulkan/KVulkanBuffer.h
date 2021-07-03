#pragma once
#include "Internal/KBufferBase.h"
#include "KVulkanConfig.h"
#include "KVulkanHeapAllocator.h"

class KVulkanVertexBuffer : public KVertexBufferBase
{
protected:
	VkBuffer m_vkBuffer;
	KVulkanHeapAllocator::AllocInfo m_AllocInfo;
	std::vector<char> m_ShadowData;
	bool m_bHostVisible;
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

	inline VkBuffer GetVulkanHandle() { return m_vkBuffer; }
	VkDeviceAddress GetDeviceAddress() const;
};

class KVulkanIndexBuffer : public KIndexBufferBase
{
protected:
	VkBuffer m_vkBuffer;
	KVulkanHeapAllocator::AllocInfo m_AllocInfo;
	std::vector<char> m_ShadowData;
	bool m_bHostVisible;
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

	inline VkBuffer GetVulkanHandle() { return m_vkBuffer; }
	inline VkIndexType GetVulkanIndexType() { return m_IndexType == IT_16 ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32; }
	VkDeviceAddress GetDeviceAddress() const;
};

class KVulkanUniformBuffer : public KUniformBufferBase
{
protected:
	VkBuffer m_vkBuffer;
	KVulkanHeapAllocator::AllocInfo m_AllocInfo;
	bool m_bDeviceInit;
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

	inline VkBuffer GetVulkanHandle() { return m_vkBuffer; }
};