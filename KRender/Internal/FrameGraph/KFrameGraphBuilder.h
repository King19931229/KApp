#pragma once
#include "Interface/IKRenderDevice.h"
#include "KFrameGraphHandle.h"

class KFrameGraphResource;
class KFrameGraphPass;

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

	bool Read(KFrameGraphPass* pass, KFrameGraphHandlePtr handle);
	bool Write(KFrameGraphPass* pass, KFrameGraphHandlePtr handle);
};