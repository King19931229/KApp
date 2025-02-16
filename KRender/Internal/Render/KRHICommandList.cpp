#include "KRHICommandList.h"
#include "Interface/IKCommandBuffer.h"
#include "Interface/IKComputePipeline.h"
#include "Interface/IKQueue.h"
#include "Interface/IKRayTracePipeline.h"
#include "Interface/IKSwapChain.h"
#include "Internal/KRenderGlobal.h"

KRHIThread::KRHIThread()
	: m_Stop(false)
{
}

KRHIThread::~KRHIThread()
{
}

void KRHIThread::StartUp()
{
	m_Stop = false;
}

void KRHIThread::Run()
{
	GetTaskGraphManager()->AttachToThread(NamedThread::RHI_THREAD);
	while (!m_Stop)
	{
		GetTaskGraphManager()->ProcessTaskUntilIdle(NamedThread::RHI_THREAD);
	}
}

void KRHIThread::ShutDown()
{
	m_Stop = true;
}

void KBeginDebugMarkerCmd::Execute(KRHICommandList& commandList)
{
	commandBuffer->BeginDebugMarker(marker, color);
}

void KEndDebugMarkerCmd::Execute(KRHICommandList& commandList)
{
	commandBuffer->EndDebugMarker();
}

void KRayTraceExecuteCmd::Execute(KRHICommandList& commandList)
{
	rayTrace->Execute(commandBuffer);
}

void KComputeExecuteCmd::Execute(KRHICommandList& commandList)
{
	compute->Execute(commandBuffer, groupX, groupY, groupZ, hasDynamicUsage ? &dynamicUsage : nullptr);
}

void KComputeExecuteIndirectCmd::Execute(KRHICommandList& commandList)
{
	compute->ExecuteIndirect(commandBuffer, indirectBuffer, hasDynamicUsage ? &dynamicUsage : nullptr);
}

void KSetViewportCmd::Execute(KRHICommandList& commandList)
{
	commandBuffer->SetViewport(area);
}

void KSetDepthBiasCmd::Execute(KRHICommandList& commandList)
{
	commandBuffer->SetDepthBias(depthBiasConstant, depthBiasClamp, depthBiasSlope);
}

void KBeginPrimaryCommandBufferCmd::Execute(KRHICommandList& commandList)
{
	commandBuffer->BeginPrimary();
}

void KEndPrimaryCommandBufferCmd::Execute(KRHICommandList& commandList)
{
	commandBuffer->End();
}

void KFlushCmd::Execute(KRHICommandList& commandList)
{
	commandBuffer->Flush();
}

void KBeginRenderPassCmd::Execute(KRHICommandList& commandList)
{
	commandBuffer->BeginRenderPass(renderPass, content);
}

void KClearColorCmd::Execute(KRHICommandList& commandList)
{
	commandBuffer->ClearColor(attachment, area, color);
}

void KClearDepthStencilCmd::Execute(KRHICommandList& commandList)
{
	commandBuffer->ClearDepthStencil(area, depthStencil);
}

void KEndRenderPassCmd::Execute(KRHICommandList& commandList)
{
	commandBuffer->EndRenderPass();
}

void KBeginQueryCmd::Execute(KRHICommandList& commandList)
{
	commandBuffer->BeginQuery(query);
}

void KEndQueryCmd::Execute(KRHICommandList& commandList)
{
	commandBuffer->EndQuery(query);
}

void KResetQueryCmd::Execute(KRHICommandList& commandList)
{
	commandBuffer->ResetQuery(query);
}

void KTransitionIndirectCmd::Execute(KRHICommandList& commandList)
{
	commandBuffer->TransitionIndirect(buf);
}

void KTransitionCmd::Execute(KRHICommandList& commandList)
{
	commandBuffer->Transition(buf, srcStages, dstStages, oldLayout, newLayout);
}

void KTransitionOwnershipCmd::Execute(KRHICommandList& commandList)
{
	commandBuffer->TransitionOwnership(buf, srcQueue, dstQueue, srcStages, dstStages, oldLayout, newLayout);
}

void KTransitionMipmapCmd::Execute(KRHICommandList& commandList)
{
	commandBuffer->TransitionMipmap(buf, mipmap, srcStages, dstStages, oldLayout, newLayout);
}

void KBlitCmd::Execute(KRHICommandList& commandList)
{
	commandBuffer->Blit(src, dest);
}

void KBeginThreadedRenderCmd::Execute(KRHICommandList& commandList)
{
	commandList.InternalCurrentThreadedContext(threadNum, commandBuffer, threadCommandPools, threadPool, renderPass, std::move(renderCmdList));
}

void KEndThreadedRenderCmd::Execute(KRHICommandList& commandList)
{
	commandList.InternalExecuteThreadedCommandBuffer();
}

void KSetThreadedRenderJobCmd::Execute(KRHICommandList& commandList)
{
	commandList.InternalSetThreadRenderJob(threadIndex, renderJob);
}

void KAddLowLevelRenderJobCmd::Execute(KRHICommandList& commandList)
{
	job(commandBuffer);
}

void KQueueSubmitCmd::Execute(KRHICommandList& commandList)
{
	queue->Submit(commandBuffer, waits, singals, fence);
}

void KSwapChainPresentCmd::Execute(KRHICommandList& commandList)
{
	bool needResize = false;
	swapChain->PresentQueue(needResize);
	if (needResize)
	{
		KRenderGlobal::RenderDevice->RecreateSwapChain(swapChain);
	}
	callback(swapChain, needResize);
}

void KUpdateUniformBufferCmd::Execute(KRHICommandList& commandList)
{
	void* pData = nullptr;
	uniformBuffer->Map(&pData);
	memcpy(POINTER_OFFSET(pData, offset), data.data(), size);
	uniformBuffer->UnMap();
}

void KUpdateStorageBufferCmd::Execute(KRHICommandList& commandList)
{
	void* pData = nullptr;
	storageBuffer->Map(&pData);
	memcpy(POINTER_OFFSET(pData, offset), data.data(), size);
	storageBuffer->UnMap();
}

void KRenderCmd::Execute(KRHICommandList& commandList)
{
	commandBuffer->Render(command);
}

void KRenderDeviceTickCmd::Execute(KRHICommandList& commandList)
{
	KRenderGlobal::RenderDevice->Tick();
}

void KRHICommandList::InternalCurrentThreadedContext(uint32_t threadNum, IKCommandBufferPtr inCommandBuffer, const std::vector<IKCommandPoolPtr>& threadCommandPools, KRenderJobExecuteThreadPool* threadPool, IKRenderPassPtr renderPass, KRenderCommandList&& renderCmdList)
{
	m_CurrentThreadNum = threadNum;
	m_CurrentCommandBuffer = inCommandBuffer;
	m_CurrentMultiThreadPool = m_MultiThreadPool;
	m_CurrentThreadCommandPools = threadCommandPools;
	m_CurrentThreadedRenderPass = renderPass;
	m_CurrentRenderCmdList = std::move(renderCmdList);
	assert(threadCommandPools.size() >= m_CurrentThreadNum);
	m_CurrentThreadedCommandBuffers.resize(m_CurrentThreadNum);
	for (uint32_t threadIndex = 0; threadIndex < m_CurrentThreadNum; ++threadIndex)
	{
		m_CurrentThreadedCommandBuffers[threadIndex] = m_CurrentThreadCommandPools[threadIndex]->Request(CBL_SECONDARY);
	}
}

void KRHICommandList::InternalSetThreadRenderJob(uint32_t threadIndex, ThreadRenderJobType job)
{
	m_CurrentMultiThreadPool->AddJob(threadIndex, [this, job, threadIndex]()
	{
		m_CurrentThreadedCommandBuffers[threadIndex]->BeginSecondary(m_CurrentThreadedRenderPass);
		job(*this, m_CurrentThreadedCommandBuffers[threadIndex], m_CurrentThreadedRenderPass, m_CurrentRenderCmdList);
		m_CurrentThreadedCommandBuffers[threadIndex]->End();
	});
}

void KRHICommandList::InternalExecuteThreadedCommandBuffer()
{
	m_CurrentMultiThreadPool->WaitAll();
	m_CurrentCommandBuffer->ExecuteAll(m_CurrentThreadedCommandBuffers);

	m_CurrentThreadNum = 0;
	m_CurrentCommandBuffer = nullptr;
	m_CurrentMultiThreadPool = nullptr;
	m_CurrentThreadedRenderPass = nullptr;
	m_CurrentThreadCommandPools.clear();
	m_CurrentThreadedCommandBuffers.clear();
}

KRHICommandList::KRHICommandList()
	: m_MultiThreadPool(nullptr)
	, m_CommandHead(nullptr)
	, m_CommandNext(&m_CommandHead)
	, m_ImmediateMode(true)
	, m_CurrentThreadNum(0)
	, m_CurrentMultiThreadPool(nullptr)
{
}

KRHICommandList::~KRHICommandList()
{
}

void KRHICommandList::SetImmediate(bool immediate)
{
	if (immediate == m_ImmediateMode)
	{
		return;
	}
	if (immediate)
	{
		Flush(RHICommandFlush::FlushRHIThread);
	}
	m_ImmediateMode = immediate;
}

void KRHICommandList::Flush(RHICommandFlush::Type flushType)
{
	if (m_ImmediateMode)
	{
		return;
	}

	if (flushType >= RHICommandFlush::DispatchToRHIThread)
	{
		if (m_AsyncTask.Get() && !m_AsyncTask->IsCompleted())
		{
			IKGraphTaskEventRef task = GetTaskGraphManager()->CreateAndDispatch(IKTaskWorkPtr(new KRHICommandTaskWork(*this, m_CommandHead)),
				NamedThread::RHI_THREAD, { m_AsyncTask });
			m_AsyncTask = task;
		}
		else
		{
			m_AsyncTask = GetTaskGraphManager()->CreateAndDispatch(IKTaskWorkPtr(new KRHICommandTaskWork(*this, m_CommandHead)), NamedThread::RHI_THREAD, {});
		}
		m_CommandHead = nullptr;
		m_CommandNext = &m_CommandHead;
	}
	if (flushType >= RHICommandFlush::FlushRHIThread)
	{
		if (m_AsyncTask.Get())
		{
			if (!m_AsyncTask->IsCompleted())
			{
				m_AsyncTask->WaitForCompletion();
			}
			m_AsyncTask.Release();
		}
	}
}

void KRHICommandList::Execute(IKRayTracePipelinePtr rayTrace)
{
	KRHICommandBasePtr command = KRHICommandBasePtr(KNEW KRayTraceExecuteCmd(m_CommandBuffer, rayTrace));
	ExecuteOrInsertNextCommand(command);
}

void KRHICommandList::Execute(IKComputePipelinePtr compute, uint32_t groupX, uint32_t groupY, uint32_t groupZ, const KDynamicConstantBufferUsage* usage)
{
	KRHICommandBasePtr command = KRHICommandBasePtr(KNEW KComputeExecuteCmd(m_CommandBuffer, compute, groupX, groupY, groupZ, usage));
	ExecuteOrInsertNextCommand(command);
}

void KRHICommandList::ExecuteIndirect(IKComputePipelinePtr compute, IKStorageBufferPtr indirectBuffer, const KDynamicConstantBufferUsage* usage)
{
	KRHICommandBasePtr command = KRHICommandBasePtr(KNEW KComputeExecuteIndirectCmd(m_CommandBuffer, compute, indirectBuffer, usage));
	ExecuteOrInsertNextCommand(command);
}

void KRHICommandList::SetViewport(const KViewPortArea& area)
{
	KRHICommandBasePtr command = KRHICommandBasePtr(KNEW KSetViewportCmd(m_CommandBuffer, area));
	ExecuteOrInsertNextCommand(command);
}

void KRHICommandList::SetDepthBias(float depthBiasConstant, float depthBiasClamp, float depthBiasSlope)
{
	KRHICommandBasePtr command = KRHICommandBasePtr(KNEW KSetDepthBiasCmd(m_CommandBuffer, depthBiasConstant, depthBiasClamp, depthBiasSlope));
	ExecuteOrInsertNextCommand(command);
}

void KRHICommandList::Render(const KRenderCommand& renderCommand)
{
	KRHICommandBasePtr command = KRHICommandBasePtr(KNEW KRenderCmd(m_CommandBuffer, renderCommand));
	ExecuteOrInsertNextCommand(command);
}

void KRHICommandList::BeginRecord()
{
	KRHICommandBasePtr command = KRHICommandBasePtr(KNEW KBeginPrimaryCommandBufferCmd(m_CommandBuffer));
	ExecuteOrInsertNextCommand(command);
}

void KRHICommandList::EndRecord()
{
	KRHICommandBasePtr command = KRHICommandBasePtr(KNEW KEndPrimaryCommandBufferCmd(m_CommandBuffer));
	ExecuteOrInsertNextCommand(command);
}

void KRHICommandList::FlushRecordDone()
{
	KRHICommandBasePtr command = KRHICommandBasePtr(KNEW KFlushCmd(m_CommandBuffer));
	ExecuteOrInsertNextCommand(command);
}

void KRHICommandList::BeginRenderPass(IKRenderPassPtr renderPass, SubpassContents content)
{
	KRHICommandBasePtr command = KRHICommandBasePtr(KNEW KBeginRenderPassCmd(m_CommandBuffer, renderPass, content));
	ExecuteOrInsertNextCommand(command);
}

void KRHICommandList::ClearColor(uint32_t attachment, const KViewPortArea& area, const KClearColor& color)
{
	KRHICommandBasePtr command = KRHICommandBasePtr(KNEW KClearColorCmd(m_CommandBuffer, attachment, area, color));
	ExecuteOrInsertNextCommand(command);
}

void KRHICommandList::ClearDepthStencil(const KViewPortArea& area, const KClearDepthStencil& depthStencil)
{
	KRHICommandBasePtr command = KRHICommandBasePtr(KNEW KClearDepthStencilCmd(m_CommandBuffer, area, depthStencil));
	ExecuteOrInsertNextCommand(command);
}

void KRHICommandList::EndRenderPass()
{
	KRHICommandBasePtr command = KRHICommandBasePtr(KNEW KEndRenderPassCmd(m_CommandBuffer));
	ExecuteOrInsertNextCommand(command);
}

void KRHICommandList::BeginDebugMarker(const std::string& marker, const glm::vec4& color)
{
	KRHICommandBasePtr command = KRHICommandBasePtr(KNEW KBeginDebugMarkerCmd(m_CommandBuffer, marker, color));
	ExecuteOrInsertNextCommand(command);
}

void KRHICommandList::EndDebugMarker()
{
	KRHICommandBasePtr command = KRHICommandBasePtr(KNEW KEndDebugMarkerCmd(m_CommandBuffer));
	ExecuteOrInsertNextCommand(command);
}

void KRHICommandList::BeginQuery(IKQueryPtr query)
{
	KRHICommandBasePtr command = KRHICommandBasePtr(KNEW KBeginQueryCmd(m_CommandBuffer, query));
	ExecuteOrInsertNextCommand(command);
}

void KRHICommandList::EndQuery(IKQueryPtr query)
{
	KRHICommandBasePtr command = KRHICommandBasePtr(KNEW KEndQueryCmd(m_CommandBuffer, query));
	ExecuteOrInsertNextCommand(command);
}

void KRHICommandList::ResetQuery(IKQueryPtr query)
{
	KRHICommandBasePtr command = KRHICommandBasePtr(KNEW KResetQueryCmd(m_CommandBuffer, query));
	ExecuteOrInsertNextCommand(command);
}

void KRHICommandList::TransitionIndirect(IKStorageBufferPtr buf)
{
	KRHICommandBasePtr command = KRHICommandBasePtr(KNEW KTransitionIndirectCmd(m_CommandBuffer, buf));
	ExecuteOrInsertNextCommand(command);
}

void KRHICommandList::Transition(IKFrameBufferPtr buf, PipelineStages srcStages, PipelineStages dstStages, ImageLayout oldLayout, ImageLayout newLayout)
{
	KRHICommandBasePtr command = KRHICommandBasePtr(KNEW KTransitionCmd(m_CommandBuffer, buf, srcStages, dstStages, oldLayout, newLayout));
	ExecuteOrInsertNextCommand(command);
}

void KRHICommandList::TransitionOwnership(IKFrameBufferPtr buf, IKQueuePtr srcQueue, IKQueuePtr dstQueue, PipelineStages srcStages, PipelineStages dstStages, ImageLayout oldLayout, ImageLayout newLayout)
{
	KRHICommandBasePtr command = KRHICommandBasePtr(KNEW KTransitionOwnershipCmd(m_CommandBuffer, buf, srcQueue, dstQueue, srcStages, dstStages, oldLayout, newLayout));
	ExecuteOrInsertNextCommand(command);
}

void KRHICommandList::TransitionMipmap(IKFrameBufferPtr buf, uint32_t mipmap, PipelineStages srcStages, PipelineStages dstStages, ImageLayout oldLayout, ImageLayout newLayout)
{
	KRHICommandBasePtr command = KRHICommandBasePtr(KNEW KTransitionMipmapCmd(m_CommandBuffer, buf, mipmap, srcStages, dstStages, oldLayout, newLayout));
	ExecuteOrInsertNextCommand(command);
}

void KRHICommandList::Blit(IKFrameBufferPtr src, IKFrameBufferPtr dest)
{
	KRHICommandBasePtr command = KRHICommandBasePtr(KNEW KBlitCmd(m_CommandBuffer, src, dest));
	ExecuteOrInsertNextCommand(command);
}

void KRHICommandList::UpdateUniformBuffer(IKUniformBufferPtr uniformBuffer, void* data, uint32_t offset, uint32_t size)
{
	KRHICommandBasePtr command = KRHICommandBasePtr(KNEW KUpdateUniformBufferCmd(uniformBuffer, data, offset, size));
	ExecuteOrInsertNextCommand(command);
}

void KRHICommandList::UpdateStorageBuffer(IKStorageBufferPtr storageBuffer, void* data, uint32_t offset, uint32_t size)
{
	KRHICommandBasePtr command = KRHICommandBasePtr(KNEW KUpdateStorageBufferCmd(storageBuffer, data, offset, size));
	ExecuteOrInsertNextCommand(command);
}

void KRHICommandList::BeginThreadedRender(uint32_t threadNum, IKRenderPassPtr renderPass, KRenderCommandList&& renderCmdList)
{
	KRHICommandBasePtr command = KRHICommandBasePtr(KNEW KBeginThreadedRenderCmd(threadNum, m_CommandBuffer, m_ThreadCommandPools, m_MultiThreadPool, renderPass, std::move(renderCmdList)));
	ExecuteOrInsertNextCommand(command);
}

void KRHICommandList::EndThreadedRender()
{
	KRHICommandBasePtr command = KRHICommandBasePtr(KNEW KEndThreadedRenderCmd());
	ExecuteOrInsertNextCommand(command);
}

void KRHICommandList::SetThreadedRenderJob(uint32_t threadIndex, ThreadRenderJobType job)
{
	KRHICommandBasePtr command = KRHICommandBasePtr(KNEW KSetThreadedRenderJobCmd(threadIndex, job));
	ExecuteOrInsertNextCommand(command);
}

void KRHICommandList::AddLowLevelRenderJob(LowLevelRenderJobType job)
{
	KRHICommandBasePtr command = KRHICommandBasePtr(KNEW KAddLowLevelRenderJobCmd(m_CommandBuffer, job));
	ExecuteOrInsertNextCommand(command);
}

void KRHICommandList::QueueSubmit(IKQueuePtr queue, std::vector<IKSemaphorePtr> waits, std::vector<IKSemaphorePtr> singals, IKFencePtr fence)
{
	KRHICommandBasePtr command = KRHICommandBasePtr(KNEW KQueueSubmitCmd(m_CommandBuffer, queue, waits, singals, fence));
	ExecuteOrInsertNextCommand(command);
}

void KRHICommandList::Present(IKSwapChain* swapChain, SwapChainResizeCallbackType callback)
{
	KRHICommandBasePtr command = KRHICommandBasePtr(KNEW KSwapChainPresentCmd(swapChain, callback));
	ExecuteOrInsertNextCommand(command);
}

void KRHICommandList::TickRenderDevice()
{
	KRHICommandBasePtr command = KRHICommandBasePtr(KNEW KRenderDeviceTickCmd());
	ExecuteOrInsertNextCommand(command);
}