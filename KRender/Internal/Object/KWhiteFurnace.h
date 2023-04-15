#pragma once

#include "Interface/IKRenderDevice.h"

class KWhiteFurnace
{
protected:
	IKComputePipelinePtr m_WFTestPipeline;
	IKRenderTargetPtr m_WFTarget;
public:
	KWhiteFurnace();
	~KWhiteFurnace();

	bool Init();
	bool UnInit();
	bool Execute();
};