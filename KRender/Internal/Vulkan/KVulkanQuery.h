#pragma once
#include "Interface/IKQuery.h"
#include "KBase/Publish/KTimer.h"
#include "KVulkanConfig.h"

class KVulkanCommandBuffer;

class KVulkanQuery : public IKQuery
{
	friend class KVulkanCommandBuffer;
protected:
	VkQueryPool m_QueryPool;
	VkQueryType m_QueryType;
	uint32_t samples;
	QueryStatus m_Status;
	KTimer m_Timer;

	inline VkQueryPool GetVKHandle() { return m_QueryPool; }
public:
	KVulkanQuery();
	~KVulkanQuery();

	virtual bool Init(QueryType type);
	virtual bool UnInit();
	virtual QueryStatus GetStatus();

	virtual bool GetResultSync(uint32_t& result);
	virtual bool GetResultAsync(uint32_t& result);
	virtual float GetElapseTime();
	virtual bool Abort();

	void Begin(VkCommandBuffer commandBuffer);
	void End(VkCommandBuffer commandBuffer);
	void Reset(VkCommandBuffer commandBuffer);
};