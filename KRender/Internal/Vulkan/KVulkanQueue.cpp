#include "KVulkanQueue.h"
#include "KVulkanCommandBuffer.h"
#include "KVulkanGlobal.h"
#include "KVulkanHelper.h"
#include "Internal/KRenderGlobal.h"
#include "KBase/Interface/IKLog.h"

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
	std::lock_guard<std::mutex> lock(m_Mutex);
	VkFenceCreateInfo fenceInfo = {};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = singaled ? VK_FENCE_CREATE_SIGNALED_BIT : VK_FLAGS_NONE;
	const VkResult result = VK_CHECK_OOM(vkCreateFence(KVulkanGlobal::device, &fenceInfo, nullptr, &m_Handle));
	m_Status = result == VK_SUCCESS ? (singaled ? Status::Signaled : Status::Pending) : Status::Failed;
	return result == VK_SUCCESS;
}

bool KVulkanFence::UnInit()
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	UpdateStatus();
	if (m_Status == Status::Pending)
	{
		m_Status = Status::Failed;
	}
	if (m_Handle != VK_NULL_HANDLE)
	{
		vkDestroyFence(KVulkanGlobal::device, m_Handle, nullptr);
		m_Handle = VK_NULL_HANDLE;
	}
	m_Payload = {};
	return true;
}

bool KVulkanFence::SetDebugName(const char* name)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	if (m_Handle != VK_NULL_HANDLE)
	{
		KVulkanHelper::DebugUtilsSetObjectName(KVulkanGlobal::device, (uint64_t)m_Handle, VK_OBJECT_TYPE_FENCE, name);
	}
	return true;
}

bool KVulkanFence::Wait()
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	return WaitLocked();
}

bool KVulkanFence::WaitLocked()
{
	const Status status = m_Status.load();
	if (status != Status::Pending || !m_Payload.submitted)
	{
		return status == Status::Signaled;
	}
	const VkResult result = VK_CHECK_OOM(vkWaitForFences(KVulkanGlobal::device, 1, &m_Handle, VK_TRUE, UINT64_MAX));
	m_Status = result == VK_SUCCESS ? Status::Signaled : Status::Failed;
	return result == VK_SUCCESS;
}

bool KVulkanFence::Reset()
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	if (m_Handle == VK_NULL_HANDLE)
	{
		return false;
	}
	const VkResult result = VK_CHECK_OOM(vkResetFences(KVulkanGlobal::device, 1, &m_Handle));
	m_Status = result == VK_SUCCESS ? Status::Pending : Status::Failed;
	if (result == VK_SUCCESS)
	{
		m_Payload = {};
	}
	return result == VK_SUCCESS;
}

void KVulkanFence::UpdateStatus() const
{
	if (m_Status == Status::Pending && m_Payload.submitted && m_Handle != VK_NULL_HANDLE)
	{
		const VkResult result = vkGetFenceStatus(KVulkanGlobal::device, m_Handle);
		if (result == VK_SUCCESS)
		{
			m_Status = Status::Signaled;
		}
		else if (result != VK_NOT_READY)
		{
			m_Status = Status::Failed;
		}
	}
}

IKFence::Status KVulkanFence::GetStatus() const
{
	// A concurrent Wait may hold the handle lock until the GPU signals it.
	std::unique_lock<std::mutex> lock(m_Mutex, std::try_to_lock);
	if (lock.owns_lock())
	{
		UpdateStatus();
	}
	return m_Status.load();
}

VkResult KVulkanFence::Submit(VkQueue queue, const VkSubmitInfo& info, uint32_t frameNum)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	if (m_Status != Status::Pending || m_Payload.submitted)
	{
		return VK_ERROR_INITIALIZATION_FAILED;
	}
	const VkResult result = m_Handle != VK_NULL_HANDLE
		? KVulkanGlobal::QueueSubmit(queue, 1, &info, m_Handle)
		: VK_ERROR_INITIALIZATION_FAILED;
	if (result == VK_SUCCESS)
	{
		m_Payload = { frameNum, true };
	}
	else
	{
		m_Status = Status::Failed;
	}
	return result;
}

void KVulkanFence::SetFailed()
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	if (m_Status == Status::Pending && !m_Payload.submitted)
	{
		m_Status = Status::Failed;
	}
}

bool KVulkanFence::WaitForFrame(uint32_t& frameNum)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	if (!m_Payload.submitted)
	{
		return false;
	}
	const bool waited = WaitLocked();
	ASSERT_RESULT(waited);
	if (!waited)
	{
		return false;
	}
	frameNum = m_Payload.frameNum;
	m_Payload.submitted = false;
	return true;
}

bool KVulkanFence::InitSlots(std::vector<IKFencePtr>& fences, const char* name)
{
	fences.resize(KRenderGlobal::NumFramesInFlight);
	for (size_t i = 0; i < fences.size(); ++i)
	{
		IKFencePtr& fence = fences[i];
		if (!KRenderGlobal::RenderDevice->CreateFence(fence) || !fence->Init(true))
		{
			SAFE_UNINIT_CONTAINER(fences);
			fences.clear();
			return false;
		}
		fence->SetDebugName((std::string(name) + "_" + std::to_string(i)).c_str());
	}
	return true;
}

bool KVulkanFence::ResetSlot(IKFencePtr& fence)
{
	IKFencePtr next;
	if (!KRenderGlobal::RenderDevice->CreateFence(next))
	{
		return false;
	}
	auto* previous = static_cast<KVulkanFence*>(fence.get());
	{
		std::lock_guard<std::mutex> lock(previous->m_Mutex);
		if (previous->m_Payload.submitted || previous->m_Handle == VK_NULL_HANDLE)
		{
			return false;
		}
		// Keep the old object's terminal status when the slot reuses its native handle.
		if (previous->m_Status == Status::Pending)
		{
			previous->m_Status = Status::Failed;
		}
		static_cast<KVulkanFence*>(next.get())->m_Handle = previous->m_Handle;
		previous->m_Handle = VK_NULL_HANDLE;
	}
	fence = std::move(next);
	return fence->Reset();
}

std::vector<KVulkanFence::Payload> KVulkanFence::GetFramePayloads(const std::vector<IKFencePtr>& fences)
{
	std::vector<Payload> result;
	result.reserve(fences.size());
	for (const auto& fence : fences)
	{
		auto* vulkanFence = static_cast<const KVulkanFence*>(fence.get());
		std::lock_guard<std::mutex> lock(vulkanFence->m_Mutex);
		result.push_back(vulkanFence->m_Payload);
	}
	return result;
}

void KVulkanFence::SetFramePayloads(const std::vector<IKFencePtr>& fences, const std::vector<Payload>& payloads)
{
	ASSERT_RESULT(payloads.size() == fences.size());
	for (size_t i = 0; i < fences.size() && i < payloads.size(); ++i)
	{
		auto* vulkanFence = static_cast<KVulkanFence*>(fences[i].get());
		std::lock_guard<std::mutex> lock(vulkanFence->m_Mutex);
		vulkanFence->m_Payload = payloads[i];
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
		m_QueueFamilyIndex = KVulkanGlobal::graphicsFamilyIndices[0];
		m_vkQueue = KVulkanGlobal::presentQueue;
		m_Category = category;
		m_QueueIndex = queueIndex;
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

bool KVulkanQueue::Submit(IKCommandBufferPtr buffer, std::vector<KSemaphoreWaitInfo> waits, std::vector<IKSemaphorePtr> singals, IKFencePtr fence)
{
	auto* vulkanFence = static_cast<KVulkanFence*>(fence.get());
	if (m_vkQueue)
	{
		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkCommandBuffer commandBuffer = VK_NULL_HANDEL;
		auto* vulkanBuffer = static_cast<KVulkanCommandBuffer*>(buffer.get());

		std::vector<VkPipelineStageFlags> waitStages;
		std::vector<VkSemaphore> waitSemaphores;
		std::vector<VkSemaphore> singalSemaphores;

		if (vulkanBuffer)
		{
			commandBuffer = vulkanBuffer->GetVkHandle();
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &commandBuffer;
		}

		waitSemaphores.resize(waits.size());
		waitStages.resize(waits.size());

		for (size_t i = 0; i < waits.size(); ++i)
		{
			waitSemaphores[i] = ((KVulkanSemaphore*)waits[i].semaphore.get())->GetVkSemaphore();
			ASSERT_RESULT(KVulkanHelper::PipelineStagesToVkPipelineStageFlags(waits[i].stages, waitStages[i]));
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

		const VkResult result = vulkanFence
			? vulkanFence->Submit(m_vkQueue, submitInfo, vulkanBuffer ? vulkanBuffer->GetFrameNum() : KRenderGlobal::CurrentFrameNum)
			: KVulkanGlobal::QueueSubmit(m_vkQueue, 1, &submitInfo, VK_NULL_HANDLE);
		if (result != VK_SUCCESS)
		{
			if (vulkanFence)
			{
				KG_LOGE(LM_RENDER, "Failed to submit queue with fence: %s (%d)", VK_ERROR_STRING(result), (int)result);
			}
			else
			{
				VK_ASSERT_RESULT(result);
			}
			return false;
		}
		if (vulkanBuffer)
		{
			vulkanBuffer->ReportSubmittedDraws();
		}

		return true;
	}
	if (vulkanFence)
	{
		vulkanFence->SetFailed();
	}
	return false;
}