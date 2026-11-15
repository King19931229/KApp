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
public:
	struct Payload
	{
		uint32_t frameNum = 0;
		bool submitted = false;
	};
protected:
	struct Fence
	{
		VkFence handle = VK_NULL_HANDLE;
		Payload payload;
	};
	std::vector<Fence> m_Fences;
public:
	KVulkanFence();
	~KVulkanFence();

	virtual bool Init(bool singaled);
	virtual bool UnInit();
	virtual bool SetDebugName(const char* name);
	virtual bool Wait();
	virtual bool Reset();

	VkFence GetVkFence();
	void SetFrameNum(uint32_t frameNum);
	bool TakeFrameNum(uint32_t& frameNum);
	std::vector<Payload> GetFramePayloads() const;
	void SetFramePayloads(const std::vector<Payload>& payloads);
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
	virtual bool Submit(IKCommandBufferPtr commandBuffer, std::vector<KSemaphoreWaitInfo> waits, std::vector<IKSemaphorePtr> singals, IKFencePtr fence);

	inline uint32_t GetQueueFamilyIndex() { return m_QueueFamilyIndex; }
	inline VkQueue GetVkQueue() { return m_vkQueue; }
};