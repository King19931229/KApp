#pragma once

#include "Interface/IKRenderDevice.h"

class KWhiteFurnace
{
protected:
	IKComputePipelinePtr m_WFTestPipeline;
	IKRenderTargetPtr m_WFTarget;

	IKCommandBufferPtr m_CommandBuffer;
	IKCommandPoolPtr m_CommandPool;
public:
	KWhiteFurnace();
	~KWhiteFurnace();

	bool Init();
	bool UnInit();
	bool Execute();
};