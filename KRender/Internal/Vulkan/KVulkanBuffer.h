#pragma once
#include "Internal/KBufferBase.h"
#include "Vulkan/vulkan.h"

class KVulkanVertexBuffer : public KVertexBufferBase
{
protected:
	VkBuffer m_vkBuffer;
	VkDeviceMemory m_vkDeviceMemory;
	bool m_bDeviceInit;
public:
	KVulkanVertexBuffer();
	~KVulkanVertexBuffer();

	virtual bool InitDevice();
	virtual bool UnInit();

	virtual bool Write(const void* pData);
	virtual bool Read(void* pData);

	virtual bool CopyFrom(IKVertexBufferPtr pSource);
	virtual bool CopyTo(IKVertexBufferPtr pDest);

	inline VkBuffer GetVulkanHandle() { return m_vkBuffer; }
};

class KVulkanIndexBuffer : public KIndexBufferBase
{
protected:
	VkBuffer m_vkBuffer;
	VkDeviceMemory m_vkDeviceMemory;
	bool m_bDeviceInit;
public:
	KVulkanIndexBuffer();
	virtual ~KVulkanIndexBuffer();

	virtual bool InitDevice();
	virtual bool UnInit();

	virtual bool Write(const void* pData);
	virtual bool Read(void* pData);

	virtual bool CopyFrom(IKIndexBufferPtr pSource);
	virtual bool CopyTo(IKIndexBufferPtr pDest);

	inline VkBuffer GetVulkanHandle() { return m_vkBuffer; }
	inline VkIndexType GetVulkanIndexType() { return m_IndexType == IT_16 ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32; }
};

class KVulkanUniformBuffer : public KUniformBufferBase
{
protected:
	VkBuffer m_vkBuffer;
	VkDeviceMemory m_vkDeviceMemory;
	bool m_bDeviceInit;
public:
	KVulkanUniformBuffer();
	virtual ~KVulkanUniformBuffer();

	virtual bool InitDevice();
	virtual bool UnInit();

	virtual bool Write(const void* pData);
	virtual bool Read(void* pData);

	virtual bool CopyFrom(IKUniformBufferPtr pSource);
	virtual bool CopyTo(IKUniformBufferPtr pDest);

	inline VkBuffer GetVulkanHandle() { return m_vkBuffer; }
};