#pragma once
#include "Interface/IKRenderDevice.h"

class KPostProcessManager
{
	IKRenderDevice* m_Device;
public:
	bool Init(IKRenderDevice* device, size_t width, size_t height);
	bool UnInit();
};