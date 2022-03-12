#pragma once
#include "KRender/Interface/IKRenderConfig.h"
#include "KRender/Interface/IKRenderCommand.h"

enum ComputeResourceFlag
{
	COMPUTE_RESOURCE_IN = 1 << 0,
	COMPUTE_RESOURCE_OUT = 1 << 1,
};
typedef uint32_t ComputeResourceFlags;

struct IKComputePipeline
{
	virtual ~IKComputePipeline() {}

	virtual void BindSampler(uint32_t location, IKFrameBufferPtr target, IKSamplerPtr sampler, bool dynamicWrite) = 0;
	virtual void BindStorageImage(uint32_t location, IKFrameBufferPtr target, ElementFormat format, ComputeResourceFlags flags, uint32_t mipmap, bool dynamicWrite) = 0;
	virtual void BindAccelerationStructure(uint32_t location, IKAccelerationStructurePtr as, bool dynamicWrite) = 0;
	virtual void BindUniformBuffer(uint32_t location, IKUniformBufferPtr buffer) = 0;

	virtual void BindSamplers(uint32_t location, const std::vector<IKFrameBufferPtr>& targets, const std::vector<IKSamplerPtr>& samplers, bool dynamicWrite) = 0;
	virtual void BindStorageImages(uint32_t location, const std::vector<IKFrameBufferPtr>& targets, ElementFormat format, ComputeResourceFlags flags, uint32_t mipmap, bool dynamicWrite) = 0;

	virtual void BindStorageBuffer(uint32_t location, IKStorageBufferPtr buffer, ComputeResourceFlags flags, bool dynamicWrite) = 0;

	virtual void BindDynamicUniformBuffer(uint32_t location) = 0;

	virtual bool Init(const char* szShader) = 0;
	virtual bool UnInit() = 0;
	virtual bool Execute(IKCommandBufferPtr primaryBuffer, uint32_t groupX, uint32_t groupY, uint32_t groupZ, const KDynamicConstantBufferUsage* usage = nullptr) = 0;
	virtual bool ExecuteIndirect(IKCommandBufferPtr primaryBuffer, IKStorageBufferPtr indirectBuffer, const KDynamicConstantBufferUsage* usage = nullptr) = 0;
	virtual bool ReloadShader() = 0;
};