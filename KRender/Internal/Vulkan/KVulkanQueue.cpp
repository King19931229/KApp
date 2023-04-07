#include "KVulkanQueue.h"
#include "KVulkanCommandBuffer.h"
#include "KVulkanGlobal.h"
#include "KVulkanHelper.h"
#include "Internal/KRenderGlobal.h"

KVulkanSemaphore::KVulkanSemaphore()
{
}

KVulkanSemaphore::~KVulkanSemaphore()
{
}

bool KVulkanSemaphore::Init()
{
	UnInit();
	m_Semaphores.resize(KRenderGlobal::NumFramesInFlight);
	for (size_t i = 0; i < m_Semaphores.size(); ++i)
	{
		VkSemaphoreCreateInfo semaphoreInfo = {};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		VK_ASSERT_RESULT(vkCreateSemaphore(KVulkanGlobal::device, &semaphoreInfo, nullptr, &m_Semaphores[i]));
	}
	return true;
}

bool KVulkanSemaphore::UnInit()
{
	if (m_Semaphores.size() > 0)
	{
		for (size_t i = 0; i < m_Semaphores.size(); ++i)
		{
			vkDestroySemaphore(KVulkanGlobal::device, m_Semaphores[i], nullptr);
		}
		m_Semaphores.clear();
	}
	return true;
}

bool KVulkanSemaphore::SetDebugName(const char* name)
{
	for (size_t i = 0; i < m_Semaphores.size(); ++i)
	{
		KVulkanHelper::DebugUtilsSetObjectName(KVulkanGlobal::device, (uint64_t)m_Semaphores[i], VK_OBJECT_TYPE_SEMAPHORE, (std::string(name) + "_" + std::to_string(i)).c_str());
	}
	return true;
}

VkSemaphore KVulkanSemaphore::GetVkSemaphore()
{
	if (m_Semaphores.size() > 0)
	{
		return m_Semaphores[KRenderGlobal::CurrentInFlightFrameIndex];
	}
	else
	{
		return VK_NULL_HANDEL;
	}
}

KVulkanFence::KVulkanFence()
{
}

KVulkanFence::~KVulkanFence()
{
}

bool KVulkanFence::Init(bool singaled)
{
	UnInit();
	m_Fences.resize(KRenderGlobal::NumFramesInFlight);
	for (size_t i = 0; i < m_Fences.size(); ++i)
	{		
		VkFenceCreateInfo fenceInfo = {};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = singaled ? VK_FENCE_CREATE_SIGNALED_BIT : VK_FLAGS_NONE;
		fenceInfo.pNext = nullptr;
		VK_ASSERT_RESULT(vkCreateFence(KVulkanGlobal::device, &fenceInfo, nullptr, &m_Fences[i]));
	}
	return true;
}

bool KVulkanFence::UnInit()
{
	if (m_Fences.size() > 0)
	{
		for (size_t i = 0; i < m_Fences.size(); ++i)
		{
			vkDestroyFence(KVulkanGlobal::device, m_Fences[i], nullptr);
		}
		m_Fences.clear();
	}
	return true;
}

bool KVulkanFence::SetDebugName(const char* name)
{
	for (size_t i = 0; i < m_Fences.size(); ++i)
	{
		KVulkanHelper::DebugUtilsSetObjectName(KVulkanGlobal::device, (uint64_t)m_Fences[i], VK_OBJECT_TYPE_FENCE, (std::string(name) + "_" + std::to_string(i)).c_str());
	}
	return true;
}

bool KVulkanFence::Wait()
{
	if (m_Fences.size() > 0)
	{
		VkResult result = vkWaitForFences(KVulkanGlobal::device, 1, &m_Fences[KRenderGlobal::CurrentInFlightFrameIndex], VK_TRUE, UINT64_MAX);
		return result == VK_SUCCESS;
	}
	return false;
}

bool KVulkanFence::Reset()
{
	if (m_Fences.size() > 0)
	{
		VkResult result = vkResetFences(KVulkanGlobal::device, 1, &m_Fences[KRenderGlobal::CurrentInFlightFrameIndex]);
		return result == VK_SUCCESS;
	}
	return false;
}

VkFence KVulkanFence::GetVkFence()
{
	if (m_Fences.size() > 0)
	{
		return m_Fences[KRenderGlobal::CurrentInFlightFrameIndex];
	}
	else
	{
		return VK_NULL_HANDEL;
	}
}

KVulkanQueue::KVulkanQueue()
	: m_vkQueue(nullptr)
	, m_Category(QUEUE_GRAPHICS)
	, m_QueueFamilyIndex(0)
	, m_QueueIndex(0)
{
}

KVulkanQueue::~KVulkanQueue()
{
	ASSERT_RESULT(m_vkQueue == VK_NULL_HANDEL);
}

bool KVulkanQueue::Init(QueueCategory category, uint32_t queueIndex)
{
	if (category == QUEUE_PRESENT)
	{
		assert(queueIndex == 0);
		m_vkQueue = KVulkanGlobal::presentQueue;
		return m_vkQueue != VK_NULL_HANDEL;
	}

	std::vector<uint32_t>* queueFamilyPtr = nullptr;
	std::vector<VkQueue>* queuePtr = nullptr;

	if (category == QUEUE_GRAPHICS)
	{
		queueFamilyPtr = &KVulkanGlobal::graphicsFamilyIndices;
		queuePtr = &KVulkanGlobal::graphicsQueues;
	}
	else if (category == QUEUE_COMPUTE)
	{
		queueFamilyPtr = &KVulkanGlobal::computeFamilyIndices;
		queuePtr = &KVulkanGlobal::computeQueues;
	}
	else if (category == QUEUE_TRANSFER)
	{
		queueFamilyPtr = &KVulkanGlobal::transferFamilyIndices;
		queuePtr = &KVulkanGlobal::transferQueues;
	}

	if (queuePtr && queueIndex < queuePtr->size() &&
		queueFamilyPtr && queueIndex < queueFamilyPtr->size())
	{
		// 一个家族只有一个Queue
		m_QueueFamilyIndex = (*queueFamilyPtr)[queueIndex];
		m_vkQueue = (*queuePtr)[queueIndex];
		m_Category = category;
		m_QueueIndex = queueIndex;
		return true;
	}
	else
	{
		assert(false);
		return false;
	}
}

bool KVulkanQueue::KVulkanQueue::UnInit()
{
	m_vkQueue = VK_NULL_HANDEL;
	return true;
}

QueueCategory KVulkanQueue::GetCategory() const
{
	return m_Category;
}

uint32_t KVulkanQueue::GetIndex() const
{
	return m_QueueIndex;
}

bool KVulkanQueue::Submit(IKCommandBufferPtr buffer, std::vector<IKSemaphorePtr> waits, std::vector<IKSemaphorePtr> singals, IKFencePtr fence)
{
	if (m_vkQueue)
	{
		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkCommandBuffer commandBuffer = VK_NULL_HANDEL;

		std::vector<VkPipelineStageFlags> waitStages;
		std::vector<VkSemaphore> waitSemaphores;
		std::vector<VkSemaphore> singalSemaphores;

		if (buffer)
		{
			commandBuffer = ((KVulkanCommandBuffer*)buffer.get())->GetVkHandle();
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &commandBuffer;
		}

		waitSemaphores.resize(waits.size());
		waitStages.resize(waits.size());

		for (size_t i = 0; i < waits.size(); ++i)
		{
			waitSemaphores[i] = ((KVulkanSemaphore*)waits[i].get())->GetVkSemaphore();
			waitStages[i] = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		}

		submitInfo.waitSemaphoreCount = (uint32_t)waits.size();
		submitInfo.pWaitSemaphores = waitSemaphores.data();
		submitInfo.pWaitDstStageMask = waitStages.data();

		singalSemaphores.resize(singals.size());
		for (size_t i = 0; i < singals.size(); ++i)
		{
			singalSemaphores[i] = ((KVulkanSemaphore*)singals[i].get())->GetVkSemaphore();
		}

		submitInfo.signalSemaphoreCount = (uint32_t)singals.size();
		submitInfo.pSignalSemaphores = singalSemaphores.data();

		VkFence vkFence = fence ? ((KVulkanFence*)fence.get())->GetVkFence() : VK_NULL_HANDEL;
		VK_ASSERT_RESULT(vkQueueSubmit(m_vkQueue, 1, &submitInfo, vkFence));

		return true;
	}
	return false;
}