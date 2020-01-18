#pragma once

#include "IKRenderConfig.h"

enum PipelineHandleState
{
	PIPELINE_HANDLE_STATE_UNLOADED,
	PIPELINE_HANDLE_STATE_PENDING,
	PIPELINE_HANDLE_STATE_LOADING,
	PIPELINE_HANDLE_STATE_LOADED
};

struct IKPipelineHandle
{
	virtual ~IKPipelineHandle() {}
	virtual PipelineHandleState GetState() = 0;
	virtual bool Init(IKPipelinePtr pipeline, IKRenderTargetPtr target, bool async) = 0;
	virtual bool UnInit() = 0;
	virtual bool WaitDevice() = 0;
};

enum PipelineResourceState
{
	PIPELINE_RESOURCE_UNLOADED,
	PIPELINE_RESOURCE_PENDING,
	PIPELINE_RESOURCE_LOADING,
	PIPELINE_RESOURCE_LOADED
};

struct IKPipeline : public std::enable_shared_from_this<IKPipeline>
{
	virtual ~IKPipeline() {}

	virtual PipelineResourceState GetState() = 0;

	virtual bool SetPrimitiveTopology(PrimitiveTopology topology) = 0;
	virtual bool SetVertexBinding(const VertexFormat* format, size_t count) = 0;

	virtual bool SetColorWrite(bool r, bool g, bool b, bool a) = 0;
	virtual bool SetColorBlend(BlendFactor srcFactor, BlendFactor dstFactor, BlendOperator op) = 0;
	virtual bool SetBlendEnable(bool enable) = 0;

	virtual bool SetCullMode(CullMode cullMode) = 0;
	virtual bool SetFrontFace(FrontFace frontFace) = 0;
	virtual bool SetPolygonMode(PolygonMode polygonMode) = 0;

	virtual bool SetDepthFunc(CompareFunc func, bool depthWrtie, bool depthTest) = 0;
	//virtual bool SetDepthBias(float depthBiasConstantFactor, float depthBiasClamp, float depthBiasSlopeFactor) = 0;
	virtual bool SetDepthBiasEnable(bool enable) = 0;

	virtual bool SetShader(ShaderTypeFlag shaderType, IKShaderPtr shader) = 0;
	virtual bool SetConstantBuffer(unsigned int location, ShaderTypes shaderTypes, IKUniformBufferPtr buffer) = 0;
	virtual bool SetSampler(unsigned int location, IKTexturePtr texture, IKSamplerPtr sampler) = 0;
	virtual bool SetSamplerDepthAttachment(unsigned int location, IKRenderTargetPtr target, IKSamplerPtr sampler) = 0;
	virtual bool PushConstantBlock(ShaderTypes shaderTypes, uint32_t size, uint32_t& offset) = 0;

	virtual bool Init(bool async) = 0;
	virtual bool UnInit() = 0;
	virtual bool Reload(bool async) = 0;

	virtual bool WaitDevice() = 0;
};