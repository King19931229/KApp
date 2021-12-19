#pragma once
#include "KRender/Interface/IKRenderConfig.h"
#include "KRender/Interface/IKRenderCommand.h"

enum ComputeImageFlag
{
	COMPUTE_IMAGE_IN,
	COMPUTE_IMAGE_OUT
};

struct IKComputePipeline
{
	virtual ~IKComputePipeline() {}

	virtual void BindSampler(uint32_t location, IKFrameBufferPtr target, IKSamplerPtr sampler, bool dynamicWrite) = 0;
	virtual void BindStorageImage(uint32_t location, IKFrameBufferPtr target, ElementFormat format, ComputeImageFlag flag, uint32_t mipmap, bool dynamicWrite) = 0;
	virtual void BindAccelerationStructure(uint32_t location, IKAccelerationStructurePtr as, bool dynamicWrite) = 0;
	virtual void BindUniformBuffer(uint32_t location, IKUniformBufferPtr buffer) = 0;

	virtual void BindSamplers(uint32_t location, const std::vector<IKFrameBufferPtr>& targets, const std::vector<IKSamplerPtr>& samplers, bool dynamicWrite) = 0;
	virtual void BindStorageImages(uint32_t location, const std::vector<IKFrameBufferPtr>& targets, ElementFormat format, ComputeImageFlag flag, uint32_t mipmap, bool dynamicWrite) = 0;

	virtual void BindStorageBuffer(uint32_t location, IKStorageBufferPtr buffer, bool dynamicWrite) = 0;

	virtual void BindDynamicUniformBuffer(uint32_t location) = 0;

	virtual bool Init(const char* szShader) = 0;
	virtual bool UnInit() = 0;
	virtual bool Execute(IKCommandBufferPtr primaryBuffer, uint32_t groupX, uint32_t groupY, uint32_t groupZ, uint32_t frameIndex, const KDynamicConstantBufferUsage* usage = nullptr) = 0;
	virtual bool ReloadShader() = 0;
};