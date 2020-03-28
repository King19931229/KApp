#pragma once
#include "Interface/IKRenderConfig.h"
#include "Interface/IKRenderWindow.h"
#include "Interface/IKRenderCommand.h"

typedef std::function<void(uint32_t chainIndex, uint32_t frameIndex)> KDevicePresentCallback;

struct IKRenderDevice
{
	virtual ~IKRenderDevice() {}

	virtual bool Init(IKRenderWindow* window) = 0;
	virtual bool UnInit() = 0;

	virtual bool CreateShader(IKShaderPtr& shader) = 0;

	virtual bool CreateVertexBuffer(IKVertexBufferPtr& buffer) = 0;
	virtual bool CreateIndexBuffer(IKIndexBufferPtr& buffer) = 0;
	virtual bool CreateUniformBuffer(IKUniformBufferPtr& buffer) = 0;

	virtual bool CreateTexture(IKTexturePtr& texture) = 0;
	virtual bool CreateSampler(IKSamplerPtr& sampler) = 0;
	virtual bool CreateSwapChain(IKSwapChainPtr& swapChain) = 0;

	virtual bool CreateRenderTarget(IKRenderTargetPtr& target) = 0;
	virtual bool CreatePipeline(IKPipelinePtr& pipeline) = 0;
	virtual bool CreatePipelineHandle(IKPipelineHandlePtr& pipelineHandle) = 0;

	virtual bool CreateUIOverlay(IKUIOverlayPtr& ui) = 0;

	virtual bool CreateCommandPool(IKCommandPoolPtr& pool) = 0;
	virtual bool CreateCommandBuffer(IKCommandBufferPtr& buffer) = 0;

	virtual bool Present() = 0;
	virtual bool Wait() = 0;

	virtual bool RecreateSwapChain() = 0;

	virtual IKSwapChainPtr GetCurrentSwapChain() = 0;
	virtual IKUIOverlayPtr GetCurrentUIOverlay() = 0;
	virtual uint32_t GetFrameInFlight() = 0;

	virtual bool RegisterPresentCallback(KDevicePresentCallback* callback) = 0;
	virtual bool UnRegisterPresentCallback(KDevicePresentCallback* callback) = 0;
};

EXPORT_DLL IKRenderDevicePtr CreateRenderDevice(RenderDevice platform);