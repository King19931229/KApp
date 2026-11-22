#pragma once
#include "Internal/KUIOverlayBase.h"
#include "KVulkanConfig.h"

struct KVulkanBufferTicket;

class KVulkanUIOverlay : public KUIOverlayBase
{
	KVulkanBufferTicket* m_VertexTicket = nullptr;
	KVulkanBufferTicket* m_IndexTicket = nullptr;

public:
	KVulkanUIOverlay();
	virtual ~KVulkanUIOverlay();

	bool UnInit() override;
	virtual bool Draw(IKRenderPassPtr renderPass, KRHICommandList& commandList);
};