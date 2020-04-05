#pragma once

#include "IKRenderConfig.h"

struct IKPipelineHandle
{
	virtual ~IKPipelineHandle() {}
	virtual bool Init(IKPipeline* pipeline, IKRenderTarget* target) = 0;
	virtual bool UnInit() = 0;
};

struct IKPipeline
{
	virtual ~IKPipeline() {}

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

	virtual bool CreateConstantBlock(ShaderTypes shaderTypes, uint32_t size) = 0;
	virtual bool DestroyConstantBlock() = 0;

	virtual bool Init() = 0;
	virtual bool UnInit() = 0;
	virtual bool Reload() = 0;

	virtual bool GetHandle(IKRenderTargetPtr target, IKPipelineHandlePtr& handle) = 0;
	virtual bool InvaildHandle(IKRenderTargetPtr target) = 0;
};