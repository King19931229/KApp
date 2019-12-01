#pragma once
#include "Interface/IKRenderConfig.h"
#include "Interface/IKRenderWindow.h"
#include "Interface/IKRenderCommand.h"

struct IKRenderDevice
{
	virtual ~IKRenderDevice() {}

	virtual bool Init(IKRenderWindowPtr window) = 0;
	virtual bool UnInit() = 0;

	virtual bool CreateShader(IKShaderPtr& shader) = 0;

	virtual bool CreateVertexBuffer(IKVertexBufferPtr& buffer) = 0;
	virtual bool CreateIndexBuffer(IKIndexBufferPtr& buffer) = 0;
	virtual bool CreateUniformBuffer(IKUniformBufferPtr& buffer) = 0;

	virtual bool CreateTexture(IKTexturePtr& texture) = 0;
	virtual bool CreateSampler(IKSamplerPtr& sampler) = 0;

	virtual bool CreateRenderTarget(IKRenderTargetPtr& target) = 0;
	virtual bool CreatePipeline(IKPipelinePtr& pipeline) = 0;
	virtual bool CreatePipelineHandle(IKPipelineHandlePtr& pipelineHandle) = 0;

	virtual bool CreateUIOVerlay(IKUIOverlayPtr& ui) = 0;

	// TODO abstract CommandBuffer
	virtual bool BeginRenderPass(void* commandBufferPtr, IKRenderTarget* target) = 0;
	virtual bool EndRenderPass(void* commandBufferPtr) = 0;

	virtual bool SetViewport(void* commandBufferPtr, IKRenderTarget* target) = 0;
	virtual bool SetDepthBias(void* commandBufferPtr, float depthBiasConstant, float depthBiasSlope) = 0;
	virtual bool Render(void* commandBufferPtr, size_t frameIndex, size_t threadIndex, const KRenderCommand& command) = 0;

	virtual bool CreateCommandPool(IKCommandPoolPtr& pool) = 0;
	virtual bool CreateCommandBuffer(IKCommandBufferPtr& buffer) = 0;

	virtual bool Present() = 0;
};

EXPORT_DLL IKRenderDevicePtr CreateRenderDevice(RenderDevice platform); 