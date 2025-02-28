#pragma once
#include "KRender/Interface/IKRenderConfig.h"
#include "KRender/Interface/IKMaterial.h"
#include "glm/glm.hpp"

struct IKAccelerationStructure
{
	struct BottomASTransformTuple
	{
		IKAccelerationStructurePtr as;
		glm::mat4 transform;
		IKMaterialTextureBindingPtr tex;
	};

	virtual ~IKAccelerationStructure() {}
	virtual bool InitBottomUp(VertexFormat format, IKVertexBufferPtr vertexBuffer, IKIndexBufferPtr indexBuffer) = 0;
	virtual bool InitTopDown(const std::vector<BottomASTransformTuple>& bottomASs) = 0;
	virtual bool UpdateTopDown(const std::vector<BottomASTransformTuple>& bottomASs) = 0;
	virtual bool UnInit() = 0;
	virtual bool SetDebugName(const char* name) = 0;
};