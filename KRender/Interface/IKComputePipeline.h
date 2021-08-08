#pragma once
#include "IKRenderConfig.h"

struct IKComputePipeline
{
	virtual ~IKComputePipeline() {}

	virtual void SetStorageImage(uint32_t location, IKRenderTargetPtr target, bool dynimicWrite = false) = 0;
	virtual void SetAccelerationStructure(uint32_t location, IKAccelerationStructurePtr as, bool dynimicWrite = false) = 0;

	virtual bool Init(const char* szShader) = 0;
	virtual bool UnInit() = 0;
	virtual bool Execute(IKCommandBufferPtr primaryBuffer, uint32_t frameIndex) = 0;
};