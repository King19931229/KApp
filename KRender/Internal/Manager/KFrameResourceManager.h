#pragma once
#include "Interface/IKRenderDevice.h"
#include "Internal/KConstantDefinition.h"

class KFrameResourceManager
{
protected:
	IKUniformBufferPtr m_ContantBuffers[CBT_STATIC_COUNT];
public:
	KFrameResourceManager();
	~KFrameResourceManager();

	bool Init();
	bool UnInit();

	IKUniformBufferPtr GetConstantBuffer(ConstantBufferType type);
};