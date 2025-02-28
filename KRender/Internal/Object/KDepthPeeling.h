#pragma once
#include "Interface/IKRenderDevice.h"
#include "Interface/IKStatistics.h"
#include "Internal/Render/KRHICommandList.h"

class KDepthPeeling
{
protected:
	IKRenderTargetPtr m_PeelingDepthTarget[2];
	IKRenderPassPtr m_PeelingPass[2];

	uint32_t m_Width;
	uint32_t m_Height;
	uint32_t m_PeelingLayers;
public:
	KDepthPeeling();
	~KDepthPeeling();

	bool Init(uint32_t width, uint32_t height, uint32_t layers);
	bool UnInit();
	bool Resize(uint32_t width, uint32_t height);

	bool Execute(KRHICommandList& commandList);
};