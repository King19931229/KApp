#include "KVulkanQueue.h"
#include "KVulkanCommandBuffer.h"
#include "KVulkanGlobal.h"
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

bool KVulkanQueue::Submit(IKCommandBufferPtr buffer, IKSemaphorePtr wait, IKSemaphorePtr singal)
{
	if (m_vkQueue)
	{
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT };

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkCommandBuffer commandBuffer = VK_NULL_HANDEL;
		VkSemaphore waitSemaphore = VK_NULL_HANDEL;
		VkSemaphore singalSemaphore = VK_NULL_HANDEL;

		if (buffer)
		{
			commandBuffer = ((KVulkanCommandBuffer*)buffer.get())->GetVkHandle();			
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &commandBuffer;
		}

		if (wait)
		{
			waitSemaphore = ((KVulkanSemaphore*)wait.get())->GetVkSemaphore();
			submitInfo.waitSemaphoreCount = 1;
			submitInfo.pWaitSemaphores = &waitSemaphore;
			submitInfo.pWaitDstStageMask = waitStages;
		}

		if (singal)
		{
			singalSemaphore = ((KVulkanSemaphore*)singal.get())->GetVkSemaphore();
			submitInfo.signalSemaphoreCount = 1;
			submitInfo.pSignalSemaphores = &singalSemaphore;
		}

		VK_ASSERT_RESULT(vkQueueSubmit(m_vkQueue, 1, &submitInfo, VK_NULL_HANDEL));

		return true;
	}
	return false;
}