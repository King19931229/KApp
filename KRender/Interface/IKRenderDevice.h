#pragma once
#include "KRender/Interface/IKRenderConfig.h"
#include "KRender/Interface/IKRenderWindow.h"
#include "KRender/Interface/IKRenderCommand.h"

typedef std::function<void(uint32_t chainIndex, uint32_t frameIndex)> KDevicePresentCallback;
typedef std::function<void(uint32_t width, uint32_t height)> KSwapChainRecreateCallback;
typedef std::function<void()> KDeviceInitCallback;
typedef std::function<void()> KDeviceUnInitCallback;

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

	virtual bool CreateRenderTarget(IKRenderTargetPtr& target) = 0;
	virtual bool CreatePipeline(IKPipelinePtr& pipeline) = 0;
	virtual bool CreatePipelineHandle(IKPipelineHandlePtr& pipelineHandle) = 0;

	virtual bool CreateCommandPool(IKCommandPoolPtr& pool) = 0;
	virtual bool CreateCommandBuffer(IKCommandBufferPtr& buffer) = 0;

	virtual bool CreateQuery(IKQueryPtr& query) = 0;

	virtual bool Present() = 0;
	virtual bool Wait() = 0;

	virtual bool RecreateSwapChain() = 0;

	virtual IKSwapChainPtr GetSwapChain() = 0;
	virtual IKUIOverlayPtr GetUIOverlay() = 0;
	virtual uint32_t GetNumFramesInFlight() = 0;

	virtual bool RegisterPresentCallback(KDevicePresentCallback* callback) = 0;
	virtual bool UnRegisterPresentCallback(KDevicePresentCallback* callback) = 0;

	virtual bool RegisterSwapChainRecreateCallback(KSwapChainRecreateCallback* callback) = 0;
	virtual bool UnRegisterSwapChainRecreateCallback(KSwapChainRecreateCallback* callback) = 0;

	virtual bool RegisterDeviceInitCallback(KDeviceInitCallback* callback) = 0;
	virtual bool UnRegisterDeviceInitCallback(KDeviceInitCallback* callback) = 0;

	virtual bool RegisterDeviceUnInitCallback(KDeviceUnInitCallback* callback) = 0;
	virtual bool UnRegisterDeviceUnInitCallback(KDeviceUnInitCallback* callback) = 0;
};

EXPORT_DLL IKRenderDevicePtr CreateRenderDevice(RenderDevice platform);