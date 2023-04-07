#pragma once
#include "Interface/IKQueue.h"
#include "KVulkanConfig.h"

class KVulkanSemaphore : public IKSemaphore
{
protected:
	std::vector<VkSemaphore> m_Semaphores;
public:
	KVulkanSemaphore();
	~KVulkanSemaphore();

	virtual bool Init();
	virtual bool UnInit();
	virtual bool SetDebugName(const char* name);

	VkSemaphore GetVkSemaphore();
};

class KVulkanFence : public IKFence
{
protected:
	std::vector<VkFence> m_Fences;
public:
	KVulkanFence();
	~KVulkanFence();

	virtual bool Init(bool singaled);
	virtual bool UnInit();
	virtual bool SetDebugName(const char* name);
	virtual bool Wait();
	virtual bool Reset();

	VkFence GetVkFence();
};

struct KVulkanQueue : public IKQueue
{
protected:
	VkQueue m_vkQueue;
	QueueCategory m_Category;
	uint32_t m_QueueFamilyIndex;
	uint32_t m_QueueIndex;
public:
	KVulkanQueue();
	~KVulkanQueue();

	virtual bool Init(QueueCategory category, uint32_t queueIndex);
	virtual bool UnInit();
	virtual QueueCategory GetCategory() const;
	virtual uint32_t GetIndex() const;
	virtual bool Submit(IKCommandBufferPtr commandBuffer, std::vector<IKSemaphorePtr> waits, std::vector<IKSemaphorePtr> singals, IKFencePtr fence);

	inline uint32_t GetQueueFamilyIndex() { return m_QueueFamilyIndex; }
	inline VkQueue GetVkQueue() { return m_vkQueue; }
};