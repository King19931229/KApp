#pragma once
#include "Interface/IKRenderDevice.h"

class KFrameGraphResource;

class KFrameGraphBuilder
{
protected:
	IKRenderDevice* m_Device;
public:
	KFrameGraphBuilder();
	~KFrameGraphBuilder();

	bool Init(IKRenderDevice* device);
	bool UnInit();

	bool Alloc(KFrameGraphResource* resource);
	bool Release(KFrameGraphResource* resource);
};