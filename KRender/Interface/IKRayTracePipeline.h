#pragma once
#include "IKRenderConfig.h"
#include "KBase/Publish/KImage.h"
#include "glm/matrix.hpp"

struct IKRayTracePipeline
{
	virtual ~IKRayTracePipeline() {}

	virtual bool SetShaderTable(ShaderType type, const char* szShader) = 0;
	virtual bool SetStorageImage(ElementFormat format) = 0;

	// TODO 场景加速结构不由Pipeline持有
	virtual uint32_t AddBottomLevelAS(IKAccelerationStructurePtr as, const glm::mat4& transform) = 0;
	virtual bool RemoveBottomLevelAS(uint32_t handle) = 0;
	virtual bool ClearBottomLevelAS() = 0;

	virtual bool RecreateAS() = 0;
	virtual bool ResizeImage(uint32_t width, uint32_t height) = 0;
	virtual bool ReloadShader() = 0;

	virtual IKRenderTargetPtr GetStorageTarget() = 0;
	virtual IKAccelerationStructurePtr GetTopdownAS() = 0;

	virtual bool Init(IKUniformBufferPtr cameraBuffer, uint32_t width, uint32_t height) = 0;
	virtual bool UnInit() = 0;
	virtual bool MarkASNeedUpdate() = 0;

	virtual bool Execute(IKCommandBufferPtr primaryBuffer) = 0;
};