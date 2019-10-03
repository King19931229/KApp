#pragma once

#include "IKRenderConfig.h"

struct IKPipeline
{
	virtual bool SetPrimitiveTopology(PrimitiveTopology topology) = 0;
	virtual bool SetVertexBinding(VertexInputDetail* inputDetails, unsigned int count) = 0;

	virtual bool SetColorBlend(BlendFactor srcFactor, BlendFactor dstFactor, BlendOperator op) = 0;
	virtual bool SetBlendEnable(bool enable) = 0;

	virtual bool SetCullMode(CullMode cullMode) = 0;
	virtual bool SetFrontFace(FrontFace frontFace) = 0;
	virtual bool SetPolygonMode(PolygonMode polygonMode) = 0;

	virtual bool SetShader(ShaderTypeFlag shaderType, IKShaderPtr shader) = 0;

	virtual bool SetConstantBuffer(unsigned int location, ShaderTypes shaderTypes, IKUniformBufferPtr buffer) = 0;
	virtual bool SetTextureSampler(unsigned int location, IKTexturePtr texture, IKSamplerPtr sampler) = 0;

	virtual bool PushConstantBuffer(ShaderTypes shaderTypes, IKUniformBufferPtr buffer) = 0;

	virtual bool Init(IKRenderTargetPtr target) = 0;
	virtual bool UnInit() = 0;
};