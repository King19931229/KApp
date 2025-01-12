#include "KRHICommandList.h"
#include "Internal/KRenderGlobal.h"

KRHIImmediateCommandList::KRHIImmediateCommandList()
	: m_CommandPool(nullptr)
	, m_CommandBuffer(nullptr)
{
}

KRHIImmediateCommandList::~KRHIImmediateCommandList()
{
	ASSERT_RESULT(!m_CommandPool);
	ASSERT_RESULT(!m_CommandBuffer);
}

void KRHIImmediateCommandList::Init()
{
	UnInit();
	KRenderGlobal::RenderDevice->CreateCommandPool(m_CommandPool);
	m_CommandPool->Init(QUEUE_GRAPHICS, 0, CBR_RESET_POOL);
}

void KRHIImmediateCommandList::UnInit()
{
	SAFE_UNINIT(m_CommandPool);
}

void KRHIImmediateCommandList::Execute(IKRayTracePipelinePtr rayTrace)
{
	rayTrace->Execute(m_CommandBuffer);
}

void KRHIImmediateCommandList::Execute(IKComputePipelinePtr compute, uint32_t groupX, uint32_t groupY, uint32_t groupZ, const KDynamicConstantBufferUsage* usage)
{
	compute->Execute(m_CommandBuffer, groupX, groupY, groupZ, usage);
}

void KRHIImmediateCommandList::ExecuteIndirect(IKComputePipelinePtr compute, IKStorageBufferPtr indirectBuffer, const KDynamicConstantBufferUsage* usage)
{
	compute->ExecuteIndirect(m_CommandBuffer, indirectBuffer, usage);
}

void KRHIImmediateCommandList::SetViewport(const KViewPortArea& area)
{
	m_CommandBuffer->SetViewport(area);
}

void KRHIImmediateCommandList::SetDepthBias(float depthBiasConstant, float depthBiasClamp, float depthBiasSlope)
{
	m_CommandBuffer->SetDepthBias(depthBiasConstant, depthBiasClamp, depthBiasSlope);
}

void KRHIImmediateCommandList::Render(const KRenderCommand& command)
{
	m_CommandBuffer->Render(command);
}

void KRHIImmediateCommandList::BeginRecord()
{
	ASSERT_RESULT(!m_CommandBuffer);
	m_CommandBuffer = m_CommandPool->Request(CBL_PRIMARY);
	m_CommandBuffer->BeginPrimary();
}

void KRHIImmediateCommandList::EndRecord()
{
	ASSERT_RESULT(m_CommandBuffer);
	m_CommandBuffer->End();
	m_CommandBuffer->Flush();
	m_CommandBuffer = nullptr;
	m_CommandPool->Reset();
}

void KRHIImmediateCommandList::BeginRenderPass(IKRenderPassPtr renderPass, SubpassContents content)
{
	m_CommandBuffer->BeginRenderPass(renderPass, content);
}

void KRHIImmediateCommandList::ClearColor(uint32_t attachment, const KViewPortArea& area, const KClearColor& color)
{
	m_CommandBuffer->ClearColor(attachment, area, color);
}

void KRHIImmediateCommandList::ClearDepthStencil(const KViewPortArea& area, const KClearDepthStencil& depthStencil)
{
	m_CommandBuffer->ClearDepthStencil(area, depthStencil);
}

void KRHIImmediateCommandList::EndRenderPass()
{
	m_CommandBuffer->EndRenderPass();
}

void KRHIImmediateCommandList::BeginDebugMarker(const std::string& marker, const glm::vec4& color)
{
	m_CommandBuffer->BeginDebugMarker(marker, color);
}

void KRHIImmediateCommandList::EndDebugMarker()
{
	m_CommandBuffer->EndDebugMarker();
}

void KRHIImmediateCommandList::BeginQuery(IKQueryPtr query)
{
	m_CommandBuffer->BeginQuery(query);
}

void KRHIImmediateCommandList::EndQuery(IKQueryPtr query)
{
	m_CommandBuffer->EndQuery(query);
}

void KRHIImmediateCommandList::ResetQuery(IKQueryPtr query)
{
	m_CommandBuffer->ResetQuery(query);
}

void KRHIImmediateCommandList::TransitionIndirect(IKStorageBufferPtr buf)
{
	m_CommandBuffer->TransitionIndirect(buf);
}

void KRHIImmediateCommandList::Transition(IKFrameBufferPtr buf, PipelineStages srcStages, PipelineStages dstStages, ImageLayout oldLayout, ImageLayout newLayout)
{
	m_CommandBuffer->Transition(buf, srcStages, dstStages, oldLayout, newLayout);
}

void KRHIImmediateCommandList::TransitionOwnership(IKFrameBufferPtr buf, IKQueuePtr srcQueue, IKQueuePtr dstQueue, PipelineStages srcStages, PipelineStages dstStages, ImageLayout oldLayout, ImageLayout newLayout)
{
	m_CommandBuffer->TransitionOwnership(buf, srcQueue, dstQueue, srcStages, dstStages, oldLayout, newLayout);
}

void KRHIImmediateCommandList::TransitionMipmap(IKFrameBufferPtr buf, uint32_t mipmap, PipelineStages srcStages, PipelineStages dstStages, ImageLayout oldLayout, ImageLayout newLayout)
{
	m_CommandBuffer->TransitionMipmap(buf, mipmap, srcStages, dstStages, oldLayout, newLayout);
}
	 
void KRHIImmediateCommandList::Blit(IKFrameBufferPtr src, IKFrameBufferPtr dest)
{
	m_CommandBuffer->Blit(src, dest);
}

void KRHIImmediateCommandList::UpdateUniformBuffer(IKUniformBufferPtr uniformBuffer, void* data, uint32_t offset, uint32_t size)
{
	void* pData = nullptr;
	uniformBuffer->Map(&pData);
	memcpy(POINTER_OFFSET(pData, offset), data, size);
	uniformBuffer->UnMap();
}

void KRHIImmediateCommandList::UpdateStorageBuffer(IKStorageBufferPtr storageBuffer, void* data, uint32_t offset, uint32_t size)
{
	void* pData = nullptr;
	storageBuffer->Map(&pData);
	memcpy(POINTER_OFFSET(pData, offset), data, size);
	storageBuffer->UnMap();
}