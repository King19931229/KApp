#pragma once
#include "Internal/KUIOverlayBase.h"
#include "KVulkanConfig.h"

class KVulkanUIOverlay : public KUIOverlayBase
{
public:
	KVulkanUIOverlay();
	virtual ~KVulkanUIOverlay();

	virtual bool Draw(unsigned int imageIndex, IKRenderTarget* target, IKCommandBuffer* commandBufferPtr);
};