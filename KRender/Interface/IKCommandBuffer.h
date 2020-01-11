#pragma once

#include "IKRenderCommand.h"

struct IKCommandPool
{
	virtual ~IKCommandPool() {};
	virtual bool Init(QueueFamilyIndex familyIndex) = 0;
	virtual bool UnInit() = 0;	
	virtual bool Reset() = 0;
};

typedef std::vector<IKCommandBufferPtr> KCommandBufferList;

struct IKCommandBuffer
{
	virtual ~IKCommandBuffer() {};

	virtual bool Init(IKCommandPoolPtr pool, CommandBufferLevel level) = 0;
	virtual bool UnInit() = 0;

	virtual bool SetViewport(IKRenderTargetPtr target) = 0;
	virtual bool SetDepthBias(float depthBiasConstant, float depthBiasClamp, float depthBiasSlope) = 0;

	virtual bool Render(const KRenderCommand& command) = 0;

	virtual bool Execute(IKCommandBufferPtr buffer) = 0;
	virtual bool ExecuteAll(KCommandBufferList& commandBuffers) = 0;

	virtual bool BeginPrimary() = 0;
	virtual bool BeginSecondary(IKRenderTargetPtr target) = 0;
	virtual bool End() = 0;

	virtual bool BeginRenderPass(IKRenderTargetPtr target, SubpassContents conent) = 0;
	virtual bool EndRenderPass() = 0;
};