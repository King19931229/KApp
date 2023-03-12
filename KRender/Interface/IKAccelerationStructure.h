#pragma once
#include "KRender/Interface/IKRenderConfig.h"
#include "KRender/Interface/IKMaterial.h"
#include "glm/glm.hpp"

struct IKAccelerationStructure
{
	typedef std::tuple<IKAccelerationStructurePtr, glm::mat4> BottomASTransformTuple;

	virtual bool InitBottomUp(VertexFormat format, IKVertexBufferPtr vertexBuffer, IKIndexBufferPtr indexBuffer, IKMaterialTextureBinding* textureBinding) = 0;
	virtual bool InitTopDown(const std::vector<BottomASTransformTuple>& bottomASs) = 0;
	virtual bool UpdateTopDown(const std::vector<BottomASTransformTuple>& bottomASs) = 0;
	virtual bool UnInit() = 0;
};