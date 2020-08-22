#pragma once
#include "Internal/KUIOverlayBase.h"
#include "KVulkanConfig.h"

// TODO
class KVulkanUIOverlay : public KUIOverlayBase
{
public:
	KVulkanUIOverlay();
	virtual ~KVulkanUIOverlay();

	virtual bool Draw(unsigned int imageIndex, IKRenderPassPtr renderPass, IKCommandBufferPtr commandBufferPtr);
};