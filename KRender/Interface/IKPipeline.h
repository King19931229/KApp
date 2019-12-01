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
	virtual bool SetSampler(unsigned int location, const ImageView& view, IKSamplerPtr sampler) = 0;
	virtual bool PushConstantBlock(ShaderTypes shaderTypes, uint32_t size, uint32_t& offset) = 0;

	virtual bool Init() = 0;
	virtual bool UnInit() = 0;
};