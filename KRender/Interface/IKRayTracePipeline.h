#pragma once
#include "IKRenderConfig.h"
#include "KBase/Publish/KImage.h"
#include "glm/matrix.hpp"

struct IKRayTracePipeline
{
	virtual bool SetShaderTable(ShaderType type, IKShaderPtr shader) = 0;
	virtual bool SetStorageImage(ElementFormat format) = 0;

	virtual uint32_t AddBottomLevelAS(IKAccelerationStructurePtr as, const glm::mat4& transform) = 0;
	virtual bool RemoveBottomLevelAS(uint32_t handle) = 0;
	virtual bool ClearBottomLevelAS() = 0;

	virtual bool RecreateAS() = 0;
	virtual bool ResizeImage(uint32_t width, uint32_t height) = 0;
	virtual bool ReloadShader() = 0;

	virtual IKRenderTargetPtr GetStorageTarget() = 0;

	virtual bool Init(const std::vector<IKUniformBufferPtr>& cameraBuffers, uint32_t width, uint32_t height) = 0;
	virtual bool UnInit() = 0;
	virtual bool MarkASNeedUpdate() = 0;

	virtual bool Execute(IKCommandBufferPtr primaryBuffer, uint32_t frameIndex) = 0;
};