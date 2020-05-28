#include "KVulkanQuery.h"
#include "KVulkanGlobal.h"
#include "KVulkanHelper.h"

KVulkanQuery::KVulkanQuery()
	: m_QueryPool(VK_NULL_HANDLE),
	m_QueryType(VK_QUERY_TYPE_MAX_ENUM),
	samples(0),
	m_Status(QS_IDEL)
{
}

KVulkanQuery::~KVulkanQuery()
{
	ASSERT_RESULT(m_QueryPool == VK_NULL_HANDLE);
}

bool KVulkanQuery::Init(QueryType type)
{
	UnInit();

	ASSERT_RESULT(KVulkanHelper::QueryTypeToVkQueryType(type, m_QueryType));

	ASSERT_RESULT(KVulkanGlobal::deviceReady);
	VkQueryPoolCreateInfo queryPoolInfo = {};
	queryPoolInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
	queryPoolInfo.queryType = VK_QUERY_TYPE_OCCLUSION;
	queryPoolInfo.queryCount = 1;
	VK_ASSERT_RESULT(vkCreateQueryPool(KVulkanGlobal::device, &queryPoolInfo, NULL, &m_QueryPool));

	m_Status = QS_IDEL;

	return true;
}

bool KVulkanQuery::UnInit()
{
	if (m_QueryPool != VK_NULL_HANDLE)
	{
		ASSERT_RESULT(KVulkanGlobal::deviceReady);
		vkDestroyQueryPool(KVulkanGlobal::device, m_QueryPool, nullptr);
		m_QueryPool = VK_NULL_HANDLE;
		m_Status = QS_IDEL;
	}
	return true;
}

QueryStatus KVulkanQuery::GetStatus()
{
	return m_Status;
}

bool KVulkanQuery::GetResultSync(uint32_t& result)
{
	if (m_QueryPool != VK_NULL_HANDLE)
	{
		ASSERT_RESULT(KVulkanGlobal::deviceReady);
		VkResult vkResult = vkGetQueryPoolResults(KVulkanGlobal::device,
			m_QueryPool,
			0, 1,
			sizeof(samples),
			&samples,
			sizeof(samples),
			VK_QUERY_RESULT_WAIT_BIT);

		// VK_QUERY_RESULT_WAIT_BIT 返回值必须是 VK_SUCCESS
		VK_ASSERT_RESULT(vkResult);
		result = samples;
		m_Status = QS_FINISH;
		return true;
	}
	return false;
}

bool KVulkanQuery::GetResultAsync(uint32_t& result)
{
	if (m_QueryPool != VK_NULL_HANDLE)
	{
		ASSERT_RESULT(KVulkanGlobal::deviceReady);
		VkResult vkResult = vkGetQueryPoolResults(KVulkanGlobal::device,
			m_QueryPool,
			0, 1,
			sizeof(samples),
			&samples,
			sizeof(samples),
			VK_QUERY_RESULT_PARTIAL_BIT);
		
		result = samples;

		if (vkResult == VK_SUCCESS)
		{
			m_Status = QS_FINISH;
			return true;
		}
		else
		{
			m_Status = QS_QUERYING;
			return false;
		}
	}
	return false;
}

// Custom define for better code readability
#define VK_FLAGS_NONE 0

void KVulkanQuery::Begin(VkCommandBuffer commandBuffer)
{
	if (commandBuffer != VK_NULL_HANDLE && m_QueryPool != VK_NULL_HANDLE)
	{
		vkCmdBeginQuery(commandBuffer, m_QueryPool, 0, VK_FLAGS_NONE);
		m_Status = QS_QUERYING;
	}
}

void KVulkanQuery::End(VkCommandBuffer commandBuffer)
{
	if (commandBuffer != VK_NULL_HANDLE && m_QueryPool != VK_NULL_HANDLE)
	{
		vkCmdEndQuery(commandBuffer, m_QueryPool, 0);
		m_Status = QS_QUERYING;
	}
}

void KVulkanQuery::Reset(VkCommandBuffer commandBuffer)
{
	if (commandBuffer != VK_NULL_HANDLE && m_QueryPool != VK_NULL_HANDLE)
	{
		vkCmdResetQueryPool(commandBuffer, m_QueryPool, 0, 1);
		m_Status = QS_IDEL;
	}
}