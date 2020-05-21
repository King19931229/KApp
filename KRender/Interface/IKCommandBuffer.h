#pragma once

#include "IKRenderCommand.h"

struct KClearColor
{
	float r;
	float g;
	float b;
	float a;
};

struct KClearDepthStencil
{
	float depth;
	unsigned int stencil;
};

struct KClearValue
{
	KClearColor color;
	KClearDepthStencil depthStencil;
};

struct KClearRect
{
	uint32_t width;
	uint32_t height;
};

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

	virtual bool BeginRenderPass(IKRenderTargetPtr target, SubpassContents conent, const KClearValue& clearValue) = 0;

	virtual bool ClearColor(const KClearRect& rect, const KClearColor& color) = 0;
	virtual bool ClearDepthStencil(const KClearRect& rect, const KClearDepthStencil& depthStencil) = 0;

	virtual bool EndRenderPass() = 0;
};