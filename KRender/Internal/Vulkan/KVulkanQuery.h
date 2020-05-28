#pragma once
#include "Interface/IKQuery.h"
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

	inline VkQueryPool GetVKHandle() { return m_QueryPool; }
public:
	KVulkanQuery();
	~KVulkanQuery();

	virtual bool Init(QueryType type);
	virtual bool UnInit();
	virtual QueryStatus GetStatus();

	virtual bool GetResultSync(uint32_t& result);
	virtual bool GetResultAsync(uint32_t& result);

	void Begin(VkCommandBuffer commandBuffer);
	void End(VkCommandBuffer commandBuffer);
	void Reset(VkCommandBuffer commandBuffer);
};