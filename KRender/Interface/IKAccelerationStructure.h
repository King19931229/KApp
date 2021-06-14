#pragma once
#include "KRender/Interface/IKRenderConfig.h"

struct IKAccelerationStructure
{
	virtual bool Init(VertexFormat format, IKVertexBufferPtr vertexBuffer, IKIndexBufferPtr indexBuffer) = 0;
	virtual bool UnInit() = 0;
};