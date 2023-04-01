#pragma once

#include "IKRenderCommand.h"
#include "IKRenderPass.h"

struct IKCommandPool
{
	virtual ~IKCommandPool() {};
	virtual bool Init(QueueCategory queue, uint32_t index) = 0;
	virtual bool UnInit() = 0;	
	virtual bool Reset() = 0;
};

typedef std::vector<IKCommandBufferPtr> KCommandBufferList;

struct IKCommandBuffer
{
	virtual ~IKCommandBuffer() {};

	virtual bool Init(IKCommandPoolPtr pool, CommandBufferLevel level) = 0;
	virtual bool UnInit() = 0;

	virtual bool SetViewport(const KViewPortArea& area) = 0;
	virtual bool SetDepthBias(float depthBiasConstant, float depthBiasClamp, float depthBiasSlope) = 0;

	virtual bool Render(const KRenderCommand& command) = 0;

	virtual bool Execute(IKCommandBufferPtr buffer) = 0;
	virtual bool ExecuteAll(KCommandBufferList& commandBuffers, bool clearAfterExecute = true) = 0;

	virtual bool BeginPrimary() = 0;
	virtual bool BeginSecondary(IKRenderPassPtr renderPass) = 0;
	virtual bool End() = 0;

	virtual bool Flush() = 0;

	virtual bool BeginRenderPass(IKRenderPassPtr renderPass, SubpassContents conent) = 0;

	virtual bool ClearColor(uint32_t attachment, const KViewPortArea& area, const KClearColor& color) = 0;
	virtual bool ClearDepthStencil(const KViewPortArea& area, const KClearDepthStencil& depthStencil) = 0;

	virtual bool EndRenderPass() = 0;

	virtual bool BeginDebugMarker(const std::string& marker, const glm::vec4 color) = 0;
	virtual bool EndDebugMarker() = 0;

	virtual bool BeginQuery(IKQueryPtr query) = 0;
	virtual bool EndQuery(IKQueryPtr query) = 0;
	virtual bool ResetQuery(IKQueryPtr query) = 0;

	virtual bool Translate(IKFrameBufferPtr buf, PipelineStages srcStages, PipelineStages dstStages, ImageLayout oldLayout, ImageLayout newLayout) = 0;
	virtual bool TranslateOwnership(IKFrameBufferPtr buf, IKQueuePtr srcQueue, IKQueuePtr dstQueue, PipelineStages srcStages, PipelineStages dstStages, ImageLayout oldLayout, ImageLayout newLayout) = 0;
	virtual bool TranslateMipmap(IKFrameBufferPtr buf, uint32_t mipmap, PipelineStages srcStages, PipelineStages dstStages, ImageLayout oldLayout, ImageLayout newLayout) = 0;

	virtual bool Blit(IKFrameBufferPtr src, IKFrameBufferPtr dest) = 0;
};