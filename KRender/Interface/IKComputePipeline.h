#pragma once
#include "IKRenderConfig.h"

struct IKComputePipeline
{
	virtual ~IKComputePipeline() {}

	virtual void BindStorageImage(uint32_t location, IKRenderTargetPtr target, bool input, bool dynimicWrite = true) = 0;
	virtual void BindAccelerationStructure(uint32_t location, IKAccelerationStructurePtr as, bool dynimicWrite = true) = 0;
	virtual void BindUniformBuffer(uint32_t location, IKUniformBufferPtr buffer, bool dynimicWrite = false) = 0;

	virtual bool Init(const char* szShader) = 0;
	virtual bool UnInit() = 0;
	virtual bool Execute(IKCommandBufferPtr primaryBuffer, uint32_t groupX, uint32_t groupY, uint32_t groupZ, uint32_t frameIndex) = 0;
};