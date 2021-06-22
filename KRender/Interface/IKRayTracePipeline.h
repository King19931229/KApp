#pragma once
#include "IKRenderConfig.h"
#include "KBase/Publish/KImage.h"
#include "glm/matrix.hpp"

struct IKRayTracePipeline
{
	virtual bool SetShaderTable(ShaderType type, IKShaderPtr shader) = 0;
	virtual bool SetStorgeImage(ElementFormat format, uint32_t width, uint32_t height) = 0;

	virtual uint32_t AddBottomLevelAS(IKAccelerationStructurePtr as, const glm::mat4& transform) = 0;
	virtual bool RemoveBottomLevelAS(uint32_t handle) = 0;
	virtual bool ClearBottomLevelAS() = 0;

	virtual bool RecreateAS() = 0;
	virtual bool ResizeImage(uint32_t width, uint32_t height) = 0;

	virtual bool Init(const std::vector<IKUniformBufferPtr>& cameraBuffers) = 0;
	virtual bool UnInit() = 0;
};