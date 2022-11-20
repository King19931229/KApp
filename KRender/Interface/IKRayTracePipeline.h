#pragma once
#include "IKRenderConfig.h"
#include "KBase/Publish/KImage.h"
#include "glm/matrix.hpp"

struct IKRayTracePipeline
{
	virtual ~IKRayTracePipeline() {}

	virtual bool SetShaderTable(ShaderType type, const char* szShader) = 0;
	virtual bool SetStorageImage(ElementFormat format) = 0;

	virtual bool RecreateFromAS() = 0;
	virtual bool ResizeImage(uint32_t width, uint32_t height) = 0;
	virtual bool ReloadShader() = 0;

	virtual IKRenderTargetPtr GetStorageTarget() = 0;

	virtual bool Init(IKUniformBufferPtr cameraBuffer, IKAccelerationStructurePtr topDownAS, uint32_t width, uint32_t height) = 0;
	virtual bool UnInit() = 0;
	virtual bool MarkASUpdated() = 0;

	virtual bool Execute(IKCommandBufferPtr primaryBuffer) = 0;
};