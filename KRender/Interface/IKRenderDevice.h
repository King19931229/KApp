#pragma once
#include "KRender/Interface/IKRenderConfig.h"
#include "KRender/Interface/IKRenderWindow.h"
#include "KRender/Interface/IKRenderCommand.h"

typedef std::function<void()> KDeviceInitCallback;
typedef std::function<void()> KDeviceUnInitCallback;

struct KRenderDeviceProperties
{
	bool anisotropySupport;
	bool bcSupport;
	bool etc1Support;
	bool etc2Support;
	bool astcSupport;
	bool raytraceSupport;
	bool meshShaderSupport;
	bool debugMarkerSupport;
	size_t uniformBufferMaxRange;
	size_t uniformBufferOffsetAlignment;
	size_t storageBufferOffsetAlignment;

	KRenderDeviceProperties()
	{
		anisotropySupport = false;
		bcSupport = false;
		etc1Support = false;
		etc2Support = false;
		astcSupport = false;
		raytraceSupport = false;
		meshShaderSupport = false;
		debugMarkerSupport = false;
		uniformBufferMaxRange = 512;
		uniformBufferOffsetAlignment = 8;
		storageBufferOffsetAlignment = 8;
	}
};

struct IKRenderDevice
{
	virtual ~IKRenderDevice() {}

	virtual bool Init(IKRenderWindow* window) = 0;
	virtual bool UnInit() = 0;

	virtual bool CreateSemaphore(IKSemaphorePtr& semaphore) = 0;
	virtual bool CreateFence(IKFencePtr& fence) = 0;
	virtual bool CreateQueue(IKQueuePtr& queue) = 0;

	virtual bool CreateShader(IKShaderPtr& shader) = 0;

	virtual bool CreateVertexBuffer(IKVertexBufferPtr& buffer) = 0;
	virtual bool CreateIndexBuffer(IKIndexBufferPtr& buffer) = 0;
	virtual bool CreateStorageBuffer(IKStorageBufferPtr& buffer) = 0;
	virtual bool CreateUniformBuffer(IKUniformBufferPtr& buffer) = 0;

	virtual bool CreateAccelerationStructure(IKAccelerationStructurePtr& as) = 0;

	virtual bool CreateTexture(IKTexturePtr& texture) = 0;
	virtual bool CreateSampler(IKSamplerPtr& sampler) = 0;

	virtual bool CreateRenderTarget(IKRenderTargetPtr& target) = 0;
	virtual bool CreatePipeline(IKPipelinePtr& pipeline) = 0;
	virtual bool CreatePipelineLayout(IKPipelineLayoutPtr& pipelineLayout) = 0;
	virtual bool CreatePipelineHandle(IKPipelineHandlePtr& pipelineHandle) = 0;
	virtual bool CreateRayTracePipeline(IKRayTracePipelinePtr& raytrace) = 0;

	virtual bool CreateComputePipeline(IKComputePipelinePtr& compute) = 0;

	virtual bool CreateCommandPool(IKCommandPoolPtr& pool) = 0;

	virtual bool CreateQuery(IKQueryPtr& query) = 0;
	virtual bool CreateSwapChain(IKSwapChainPtr& swapChain) = 0;
	virtual bool CreateUIOverlay(IKUIOverlayPtr& ui) = 0;
	virtual bool CreateRenderPass(IKRenderPassPtr& renderPass) = 0;

	virtual bool Wait() = 0;
	virtual bool Tick() = 0;

	virtual bool RegisterDeviceInitCallback(KDeviceInitCallback* callback) = 0;
	virtual bool UnRegisterDeviceInitCallback(KDeviceInitCallback* callback) = 0;

	virtual bool RegisterDeviceUnInitCallback(KDeviceUnInitCallback* callback) = 0;
	virtual bool UnRegisterDeviceUnInitCallback(KDeviceUnInitCallback* callback) = 0;

	virtual bool QueryProperty(KRenderDeviceProperties** ppProperty) = 0;

	virtual bool RecreateSwapChain(IKSwapChain* swapChain) = 0;
	virtual IKRenderWindow* GetMainWindow() = 0;

	virtual bool SetCheckPointMarker(IKCommandBuffer* commandBuffer, uint32_t frameNum, const char* marker) = 0;
};

EXPORT_DLL IKRenderDevicePtr CreateRenderDevice(RenderDevice platform);