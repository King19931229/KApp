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

	VkSemaphore GetVkSemaphore();
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
	virtual bool Submit(IKCommandBufferPtr commandBuffer, IKSemaphorePtr wait, IKSemaphorePtr singal);

	inline uint32_t GetQueueFamilyIndex() { return m_QueueFamilyIndex; }
	inline VkQueue GetVkQueue() { return m_vkQueue; }
};