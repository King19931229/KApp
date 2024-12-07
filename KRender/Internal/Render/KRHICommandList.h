#pragma once
#include "Interface/IKRenderDevice.h"
#include "Internal/KRenderThreadPool.h"
#include "Interface/IKRenderPass.h"
#include "Interface/IKCommandBuffer.h"
#include "Interface/IKQuery.h"
#include "KBase/Interface/Task/IKTaskGraph.h"
#include "KBase/Interface/IKRunable.h"

struct KRHICommandBase;
class KRHICommandList;

typedef std::shared_ptr<KRHICommandBase> KRHICommandBasePtr;

struct KRHICommandBase
{
	KRHICommandBasePtr next;
	KRHICommandBase()
	{
		next = nullptr;
	}
	virtual void Execute(KRHICommandList& commandList) = 0;
	virtual const char* GetName() = 0;
};

typedef std::shared_ptr<KRHICommandBase> KRHICommandBasePtr;

#define RHICOMMAND_DEFINE(CommandName)\
struct KRHI##CommandName##Base : public KRHICommandBase\
{\
	virtual const char* GetName() { return #CommandName; }\
};\
struct CommandName : public KRHI##CommandName##Base\

RHICOMMAND_DEFINE(KBeginDebugMarkerCmd)
{
	IKCommandBufferPtr commandBuffer;
	std::string marker;
	glm::vec4 color;

	KBeginDebugMarkerCmd(IKCommandBufferPtr inCommandBuffer, const std::string& inMarker, const glm::vec4& inColor)
		: commandBuffer(inCommandBuffer)
		, marker(inMarker)
		, color(inColor)
	{
	}
	virtual void Execute(KRHICommandList & commandList) override;
};

RHICOMMAND_DEFINE(KEndDebugMarkerCmd)
{
	IKCommandBufferPtr commandBuffer;

	KEndDebugMarkerCmd(IKCommandBufferPtr inCommandBuffer)
		: commandBuffer(inCommandBuffer)
	{}
	virtual void Execute(KRHICommandList& commandList);
};

RHICOMMAND_DEFINE(KRayTraceExecuteCmd)
{
	IKCommandBufferPtr commandBuffer;
	IKRayTracePipelinePtr rayTrace;

	KRayTraceExecuteCmd(IKCommandBufferPtr inCommandBuffer, IKRayTracePipelinePtr inRayTrace)
		: commandBuffer(inCommandBuffer)
		, rayTrace(inRayTrace)
	{}
	virtual void Execute(KRHICommandList& commandList);
};

RHICOMMAND_DEFINE(KComputeExecuteCmd)
{
	IKCommandBufferPtr commandBuffer;
	IKComputePipelinePtr compute;
	uint32_t groupX;
	uint32_t groupY;
	uint32_t groupZ;
	KDynamicConstantBufferUsage dynamicUsage;
	bool hasDynamicUsage;

	KComputeExecuteCmd(IKCommandBufferPtr inCommandBuffer, IKComputePipelinePtr inCompute,
		uint32_t inGroupX, uint32_t inGroupY, uint32_t inGroupZ,
		const KDynamicConstantBufferUsage* inDynamicUsage)
		: commandBuffer(inCommandBuffer), compute(inCompute)
		, groupX(inGroupX), groupY(inGroupY), groupZ(inGroupZ)
		, hasDynamicUsage(inDynamicUsage != nullptr)
	{
		if (inDynamicUsage)
		{
			dynamicUsage = *inDynamicUsage;
		}
	}
	virtual void Execute(KRHICommandList& commandList);
};

RHICOMMAND_DEFINE(KComputeExecuteIndirectCmd)
{
	IKCommandBufferPtr commandBuffer;
	IKComputePipelinePtr compute;
	IKStorageBufferPtr indirectBuffer;
	KDynamicConstantBufferUsage dynamicUsage;
	bool hasDynamicUsage;

	KComputeExecuteIndirectCmd(IKCommandBufferPtr inCommandBuffer, IKComputePipelinePtr inCompute, IKStorageBufferPtr inIndirectBuffer,
		const KDynamicConstantBufferUsage* inDynamicUsage)
		: commandBuffer(inCommandBuffer), compute(inCompute), indirectBuffer(inIndirectBuffer)
		, hasDynamicUsage(inDynamicUsage != nullptr)
	{
		if (inDynamicUsage)
		{
			dynamicUsage = *inDynamicUsage;
		}
	}
	virtual void Execute(KRHICommandList& commandList);
};

RHICOMMAND_DEFINE(KSetViewportCmd)
{
	IKCommandBufferPtr commandBuffer;
	KViewPortArea area;

	KSetViewportCmd(IKCommandBufferPtr inCommandBuffer, const KViewPortArea& inArea)
		: commandBuffer(inCommandBuffer)
		, area(inArea)
	{
	}
	virtual void Execute(KRHICommandList& commandList);
};

RHICOMMAND_DEFINE(KSetDepthBiasCmd)
{
	IKCommandBufferPtr commandBuffer;
	float depthBiasConstant;
	float depthBiasClamp;
	float depthBiasSlope;

	KSetDepthBiasCmd(IKCommandBufferPtr inCommandBuffer, float inDepthBiasConstant, float inDepthBiasClamp, float inDepthBiasSlope)
		: commandBuffer(inCommandBuffer)
		, depthBiasConstant(inDepthBiasConstant)
		, depthBiasClamp(inDepthBiasClamp)
		, depthBiasSlope(inDepthBiasSlope)
	{
	}
	virtual void Execute(KRHICommandList& commandList);
};

RHICOMMAND_DEFINE(KRenderCmd)
{
	IKCommandBufferPtr commandBuffer;
	KRenderCommand command;

	KRenderCmd(IKCommandBufferPtr inCommandBuffer, const KRenderCommand & inCommand)
		: commandBuffer(inCommandBuffer)
		, command(inCommand)
	{
	}
	virtual void Execute(KRHICommandList& commandList);
};

RHICOMMAND_DEFINE(KBeginPrimaryCommandBufferCmd)
{
	IKCommandBufferPtr commandBuffer;

	KBeginPrimaryCommandBufferCmd(IKCommandBufferPtr inCommandBuffer)
		: commandBuffer(inCommandBuffer)
	{
	}
	virtual void Execute(KRHICommandList& commandList) override;
};

RHICOMMAND_DEFINE(KEndPrimaryCommandBufferCmd)
{
	IKCommandBufferPtr commandBuffer;

	KEndPrimaryCommandBufferCmd(IKCommandBufferPtr inCommandBuffer)
		: commandBuffer(inCommandBuffer)
	{
	}
	virtual void Execute(KRHICommandList& commandList) override;
};

RHICOMMAND_DEFINE(KFlushCmd)
{
	IKCommandBufferPtr commandBuffer;

	KFlushCmd(IKCommandBufferPtr inCommandBuffer)
		: commandBuffer(inCommandBuffer)
	{
	}
	virtual void Execute(KRHICommandList& commandList) override;
};

RHICOMMAND_DEFINE(KBeginRenderPassCmd)
{
	IKCommandBufferPtr commandBuffer;
	IKRenderPassPtr renderPass;
	SubpassContents content;

	KBeginRenderPassCmd(IKCommandBufferPtr inCommandBuffer, IKRenderPassPtr inRenderPass, SubpassContents inContent)
		: commandBuffer(inCommandBuffer)
		, renderPass(inRenderPass)
		, content(inContent)
	{}

	virtual void Execute(KRHICommandList& commandList);
};

RHICOMMAND_DEFINE(KEndRenderPassCmd)
{
	IKCommandBufferPtr commandBuffer;

	KEndRenderPassCmd(IKCommandBufferPtr inCommandBuffer)
		: commandBuffer(inCommandBuffer)
	{}

	virtual void Execute(KRHICommandList& commandList);
};

RHICOMMAND_DEFINE(KClearColorCmd)
{
	IKCommandBufferPtr commandBuffer;
	uint32_t attachment;
	KViewPortArea area;
	KClearColor color;

	KClearColorCmd(IKCommandBufferPtr inCommandBuffer, uint32_t inAttachment, const KViewPortArea& inArea, const KClearColor& inColor)
		: commandBuffer(inCommandBuffer)
		, attachment(inAttachment)
		, area(inArea)
		, color(inColor)
	{}

	virtual void Execute(KRHICommandList& commandList);
};

RHICOMMAND_DEFINE(KClearDepthStencilCmd)
{
	IKCommandBufferPtr commandBuffer;
	KViewPortArea area;
	KClearDepthStencil depthStencil;

	KClearDepthStencilCmd(IKCommandBufferPtr inCommandBuffer, const KViewPortArea& inArea, const KClearDepthStencil& inDepthStencil)
		: commandBuffer(inCommandBuffer)
		, area(inArea)
		, depthStencil(inDepthStencil)
	{}

	virtual void Execute(KRHICommandList& commandList);
};

RHICOMMAND_DEFINE(KBeginQueryCmd)
{
	IKCommandBufferPtr commandBuffer;
	IKQueryPtr query;
	KBeginQueryCmd(IKCommandBufferPtr inCommandBuffer, IKQueryPtr inQuery)
		: commandBuffer(inCommandBuffer)
		, query(inQuery)
	{}

	virtual void Execute(KRHICommandList& commandList);
};

RHICOMMAND_DEFINE(KEndQueryCmd)
{
	IKCommandBufferPtr commandBuffer;
	IKQueryPtr query;
	KEndQueryCmd(IKCommandBufferPtr inCommandBuffer, IKQueryPtr inQuery)
		: commandBuffer(inCommandBuffer)
		, query(inQuery)
	{}

	virtual void Execute(KRHICommandList& commandList);
};

RHICOMMAND_DEFINE(KResetQueryCmd)
{
	IKCommandBufferPtr commandBuffer;
	IKQueryPtr query;
	KResetQueryCmd(IKCommandBufferPtr inCommandBuffer, IKQueryPtr inQuery)
		: commandBuffer(inCommandBuffer)
		, query(inQuery)
	{}

	virtual void Execute(KRHICommandList& commandList);
};

RHICOMMAND_DEFINE(KTransitionIndirectCmd)
{
	IKCommandBufferPtr commandBuffer;
	IKStorageBufferPtr buf;

	KTransitionIndirectCmd(IKCommandBufferPtr inCommandBuffer, IKStorageBufferPtr inBuf)
		: commandBuffer(inCommandBuffer)
		, buf(inBuf)
	{}

	virtual void Execute(KRHICommandList& commandList);
};

RHICOMMAND_DEFINE(KTransitionCmd)
{
	IKCommandBufferPtr commandBuffer;
	IKFrameBufferPtr buf;
	PipelineStages srcStages;
	PipelineStages dstStages;
	ImageLayout oldLayout;
	ImageLayout newLayout;

	KTransitionCmd(IKCommandBufferPtr inCommandBuffer, IKFrameBufferPtr inBuf, PipelineStages inSrcStages, PipelineStages inDstStages, ImageLayout inOldLayout, ImageLayout inNewLayout)
		: commandBuffer(inCommandBuffer)
		, buf(inBuf)
		, srcStages(inSrcStages)
		, dstStages(inDstStages)
		, oldLayout(inOldLayout)
		, newLayout(inNewLayout)
	{}

	virtual void Execute(KRHICommandList& commandList);
};

RHICOMMAND_DEFINE(KTransitionOwnershipCmd)
{
	IKCommandBufferPtr commandBuffer;
	IKFrameBufferPtr buf;
	IKQueuePtr srcQueue;
	IKQueuePtr dstQueue;
	PipelineStages srcStages;
	PipelineStages dstStages;
	ImageLayout oldLayout;
	ImageLayout newLayout;

	KTransitionOwnershipCmd(IKCommandBufferPtr inCommandBuffer, IKFrameBufferPtr inBuf, IKQueuePtr inSrcQueue, IKQueuePtr inDstQueue, PipelineStages inSrcStages, PipelineStages inDstStages, ImageLayout inOldLayout, ImageLayout inNewLayout)
		: commandBuffer(inCommandBuffer)
		, buf(inBuf)
		, srcQueue(inSrcQueue)
		, dstQueue(inDstQueue)
		, srcStages(inSrcStages)
		, dstStages(inDstStages)
		, oldLayout(inOldLayout)
		, newLayout(inNewLayout)
	{}

	virtual void Execute(KRHICommandList& commandList);
};

RHICOMMAND_DEFINE(KTransitionMipmapCmd)
{
	IKCommandBufferPtr commandBuffer;
	IKFrameBufferPtr buf;
	uint32_t mipmap;
	PipelineStages srcStages;
	PipelineStages dstStages;
	ImageLayout oldLayout;
	ImageLayout newLayout;

	KTransitionMipmapCmd(IKCommandBufferPtr inCommandBuffer, IKFrameBufferPtr inBuf, uint32_t inMipmap, PipelineStages inSrcStages, PipelineStages inDstStages, ImageLayout inOldLayout, ImageLayout inNewLayout)
		: commandBuffer(inCommandBuffer)
		, buf(inBuf)
		, mipmap(inMipmap)
		, srcStages(inSrcStages)
		, dstStages(inDstStages)
		, oldLayout(inOldLayout)
		, newLayout(inNewLayout)
	{}

	virtual void Execute(KRHICommandList& commandList);
};

RHICOMMAND_DEFINE(KBlitCmd)
{
	IKCommandBufferPtr commandBuffer;
	IKFrameBufferPtr src;
	IKFrameBufferPtr dest;

	KBlitCmd(IKCommandBufferPtr inCommandBuffer, IKFrameBufferPtr inSrc, IKFrameBufferPtr inDest)
		: commandBuffer(inCommandBuffer)
		, src(inSrc)
		, dest(inDest)
	{}

	virtual void Execute(KRHICommandList& commandList);
};

RHICOMMAND_DEFINE(KUpdateUniformBufferCmd)
{
	IKUniformBufferPtr uniformBuffer;
	std::vector<char> data;
	uint32_t offset;
	uint32_t size;

	KUpdateUniformBufferCmd(IKUniformBufferPtr inUniformBuffer, void* inData, uint32_t inOffset, uint32_t inSize)
		: uniformBuffer(inUniformBuffer)
		, offset(inOffset)
		, size(inSize)
	{
		data.resize(size);
		memcpy(data.data(), inData, inSize);
	}

	virtual void Execute(KRHICommandList& commandList) override;
};

RHICOMMAND_DEFINE(KUpdateStorageBufferCmd)
{
	IKStorageBufferPtr storageBuffer;
	std::vector<char> data;
	uint32_t offset;
	uint32_t size;

	KUpdateStorageBufferCmd(IKStorageBufferPtr inStorageBuffer, void* inData, uint32_t inOffset, uint32_t inSize)
		: storageBuffer(inStorageBuffer)
		, offset(inOffset)
		, size(inSize)
	{
		data.resize(size);
		memcpy(data.data(), inData, inSize);
	}

	virtual void Execute(KRHICommandList& commandList) override;
};

RHICOMMAND_DEFINE(KBeginThreadedRenderCmd)
{
	uint32_t threadNum;
	IKCommandBufferPtr commandBuffer;
	std::vector<IKCommandPoolPtr> threadCommandPools;
	KRenderJobExecuteThreadPool* threadPool;
	IKRenderPassPtr renderPass;
	KRenderCommandList renderCmdList;
	KBeginThreadedRenderCmd(uint32_t inThreadNum, IKCommandBufferPtr inCommandBuffer, const std::vector<IKCommandPoolPtr>& inThreadCommandPools, KRenderJobExecuteThreadPool* inThreadPool, IKRenderPassPtr inRenderPass, KRenderCommandList&& inRenderCmdList)
		: threadNum(inThreadNum)
		, commandBuffer(inCommandBuffer)
		, threadCommandPools(inThreadCommandPools)
		, threadPool(inThreadPool)
		, renderPass(inRenderPass)
		, renderCmdList(std::move(inRenderCmdList))
	{
	}
	virtual void Execute(KRHICommandList& commandList) override;
};

RHICOMMAND_DEFINE(KEndThreadedRenderCmd)
{
	KEndThreadedRenderCmd()
	{
	}
	virtual void Execute(KRHICommandList& commandList) override;
};

typedef std::function<void(KRHICommandList&, IKCommandBufferPtr, IKRenderPassPtr, KRenderCommandList&)> ThreadRenderJobType;
RHICOMMAND_DEFINE(KSetThreadedRenderJobCmd)
{
	uint32_t threadIndex;
	ThreadRenderJobType renderJob;
	KSetThreadedRenderJobCmd(uint32_t inThreadIndex, ThreadRenderJobType inRenderJob)
		: threadIndex(inThreadIndex)
		, renderJob(inRenderJob)
	{
	}
	virtual void Execute(KRHICommandList& commandList) override;
};

typedef std::function<void(IKCommandBufferPtr)> LowLevelRenderJobType;
RHICOMMAND_DEFINE(KAddLowLevelRenderJobCmd)
{
	IKCommandBufferPtr commandBuffer;
	LowLevelRenderJobType job;
	KAddLowLevelRenderJobCmd(IKCommandBufferPtr inCommandBuffer, LowLevelRenderJobType inJob)
		: commandBuffer(inCommandBuffer)
		, job(inJob)
	{}
	virtual void Execute(KRHICommandList& commandList) override;
};

RHICOMMAND_DEFINE(KQueueSubmitCmd)
{
	IKCommandBufferPtr commandBuffer;
	IKQueuePtr queue;
	std::vector<IKSemaphorePtr> waits;
	std::vector<IKSemaphorePtr> singals;
	IKFencePtr fence;

	KQueueSubmitCmd(IKCommandBufferPtr inCommandBuffer,
		IKQueuePtr inQueue,
		std::vector<IKSemaphorePtr> inWaits,
		std::vector<IKSemaphorePtr> inSingals,
		IKFencePtr inFence)
		: commandBuffer(inCommandBuffer)
		, queue(inQueue)
		, waits(inWaits)
		, singals(inSingals)
		, fence(inFence)
	{}
	virtual void Execute(KRHICommandList& commandList) override;
};

RHICOMMAND_DEFINE(KRenderDeviceTickCmd)
{
	virtual void Execute(KRHICommandList & commandList) override;
};

typedef std::function<void(IKSwapChain*, bool needResize)> SwapChainResizeCallbackType;
RHICOMMAND_DEFINE(KSwapChainPresentCmd)
{
	IKSwapChain* swapChain;
	SwapChainResizeCallbackType callback;

	KSwapChainPresentCmd(IKSwapChain* inSwapChain, SwapChainResizeCallbackType inCallback)
		: swapChain(inSwapChain)
		, callback(inCallback)
	{}
	virtual void Execute(KRHICommandList& commandList) override;
};

namespace RHICommandFlush
{
	enum Type
	{
		DispatchToRHIThread,
		FlushRHIThread,
		FlushRHIThreadToDone
	};
}

struct KRHICommandTaskWork : public IKTaskWork
{
	KRHICommandList& commandList;
	KRHICommandBasePtr commandHead;

	KRHICommandTaskWork(KRHICommandList& inCommandList, KRHICommandBasePtr inCommandHead)
		: commandList(inCommandList)
		, commandHead(inCommandHead)
	{}

	virtual void DoWork() override
	{
		while (commandHead)
		{
			commandHead->Execute(commandList);
			commandHead = commandHead->next;
		}
	}

	virtual const char* GetDebugInfo() { return "KRHICommandTaskWork"; }
};

class KRHIThread : public IKRunable
{
protected:
	bool m_Stop;
public:
	KRHIThread();
	~KRHIThread();

	virtual void StartUp() override;
	virtual void Run() override;
	virtual void ShutDown() override;
};

class KRHICommandList
{
protected:
	std::vector<IKCommandPoolPtr> m_ThreadCommandPools;
	IKCommandBufferPtr m_CommandBuffer;
	KRenderJobExecuteThreadPool* m_MultiThreadPool;
	KRHICommandBasePtr m_CommandHead;
	KRHICommandBasePtr* m_CommandNext;
	IKGraphTaskEventRef m_AsyncTask;

	bool m_ImmediateMode;

	uint32_t m_CurrentThreadNum;
	KRenderJobExecuteThreadPool* m_CurrentMultiThreadPool;
	IKRenderPassPtr m_CurrentThreadedRenderPass;
	std::vector<IKCommandPoolPtr> m_CurrentThreadCommandPools;
	IKCommandBufferPtr m_CurrentCommandBuffer;
	KCommandBufferList m_CurrentThreadedCommandBuffers;
	KRenderCommandList m_CurrentRenderCmdList;

	inline void ExecuteOrInsertNextCommand(KRHICommandBasePtr command)
	{
		if (m_ImmediateMode)
		{
			command->Execute(*this);
			return;
		}
		*m_CommandNext = command;
		m_CommandNext = &command->next;
	}
public:
	KRHICommandList();
	~KRHICommandList();

	inline void SetMultiThreadPool(KRenderJobExecuteThreadPool* multiThreadPool) { m_MultiThreadPool = multiThreadPool; }
	inline void SetThreadCommandPools(const std::vector<IKCommandPoolPtr>& threadPools) { m_ThreadCommandPools = threadPools; }
	inline void SetCommandBuffer(IKCommandBufferPtr commandBuffer) { m_CommandBuffer = commandBuffer; }

	inline uint32_t GetThreadPoolSize() const { return (uint32_t)m_ThreadCommandPools.size(); }

	void SetImmediate(bool immediate);

	void Flush(RHICommandFlush::Type flushType);

	void Execute(IKRayTracePipelinePtr rayTrace);
	void Execute(IKComputePipelinePtr compute, uint32_t groupX, uint32_t groupY, uint32_t groupZ, const KDynamicConstantBufferUsage* usage = nullptr);
	void ExecuteIndirect(IKComputePipelinePtr compute, IKStorageBufferPtr indirectBuffer, const KDynamicConstantBufferUsage* usage = nullptr);

	void SetViewport(const KViewPortArea& area);
	void SetDepthBias(float depthBiasConstant, float depthBiasClamp, float depthBiasSlope);
 
	void Render(const KRenderCommand& command);

	void BeginRecord();	
	void EndRecord();

	void FlushDoneRecord();

	void BeginRenderPass(IKRenderPassPtr renderPass, SubpassContents conent);

	void ClearColor(uint32_t attachment, const KViewPortArea& area, const KClearColor& color);
	void ClearDepthStencil(const KViewPortArea& area, const KClearDepthStencil& depthStencil);

	void EndRenderPass();

	void BeginDebugMarker(const std::string& marker, const glm::vec4& color);
	void EndDebugMarker();

	void BeginQuery(IKQueryPtr query);
	void EndQuery(IKQueryPtr query);
	void ResetQuery(IKQueryPtr query);

	void TransitionIndirect(IKStorageBufferPtr buf);

	void Transition(IKFrameBufferPtr buf, PipelineStages srcStages, PipelineStages dstStages, ImageLayout oldLayout, ImageLayout newLayout);
	void TransitionOwnership(IKFrameBufferPtr buf, IKQueuePtr srcQueue, IKQueuePtr dstQueue, PipelineStages srcStages, PipelineStages dstStages, ImageLayout oldLayout, ImageLayout newLayout);
	void TransitionMipmap(IKFrameBufferPtr buf, uint32_t mipmap, PipelineStages srcStages, PipelineStages dstStages, ImageLayout oldLayout, ImageLayout newLayout);

	void Blit(IKFrameBufferPtr src, IKFrameBufferPtr dest);

	void UpdateUniformBuffer(IKUniformBufferPtr uniformBuffer, void* data, uint32_t offset, uint32_t size);
	void UpdateStorageBuffer(IKStorageBufferPtr storageBuffer, void* data, uint32_t offset, uint32_t size);

	void BeginThreadedRender(uint32_t threadNum, IKRenderPassPtr renderPass, KRenderCommandList&& renderCmdList);
	void EndThreadedRender();
	void SetThreadedRenderJob(uint32_t threadIndex, ThreadRenderJobType job);

	void AddLowLevelRenderJob(LowLevelRenderJobType job);

	void QueueSubmit(IKQueuePtr queue, std::vector<IKSemaphorePtr> waits, std::vector<IKSemaphorePtr> singals, IKFencePtr fence);

	void TickRenderDevice();

	void Present(IKSwapChain* swapChain, SwapChainResizeCallbackType callback);

	void InternalCurrentThreadedContext(uint32_t threadNum, IKCommandBufferPtr commandBuffer, const std::vector<IKCommandPoolPtr>& threadCommandPools, KRenderJobExecuteThreadPool* threadPool, IKRenderPassPtr renderPass, KRenderCommandList&& renderCmdList);
	void InternalSetThreadRenderJob(uint32_t threadIndex, ThreadRenderJobType job);
	void InternalExecuteThreadedCommandBuffer();
};

typedef std::function<void(IKRenderPassPtr, KRHICommandList&)> RenderPassCallFuncType;