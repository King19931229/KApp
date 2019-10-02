#pragma once
#include "Interface/IKRenderConfig.h"
#include "Interface/IKRenderWindow.h"
#include "Interface/IKShader.h"

struct IKRenderDevice
{
	virtual ~IKRenderDevice() {}

	virtual bool Init(IKRenderWindowPtr window) = 0;
	virtual bool UnInit() = 0;

	virtual bool CreateShader(IKShaderPtr& shader) = 0;
	virtual bool CreateProgram(IKProgramPtr& program) = 0;

	virtual bool CreateVertexBuffer(IKVertexBufferPtr& buffer) = 0;
	virtual bool CreateIndexBuffer(IKIndexBufferPtr& buffer) = 0;
	virtual bool CreateUniformBuffer(IKUniformBufferPtr& buffer) = 0;

	virtual bool CreateTexture(IKTexturePtr& texture) = 0;
	virtual bool CreateSampler(IKSamplerPtr& sampler) = 0;

	virtual bool CreateRenderTarget(IKRenderTargetPtr& target) = 0;

	virtual bool CreatePipeline(IKPipelinePtr& pipeline) = 0;

	virtual bool Present() = 0;
};

EXPORT_DLL IKRenderDevicePtr CreateRenderDevice(RenderDevice platform); 