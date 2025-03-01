#pragma once
#include "Interface/IKRenderDevice.h"
#include "Interface/IKStatistics.h"
#include "Internal/Render/KRHICommandList.h"
#include "KBase/Interface/Entity/IKEntity.h"

class KDepthPeeling
{
protected:
	IKRenderTargetPtr m_PeelingTarget;
	IKRenderTargetPtr m_PeelingDepthTarget[2];

	IKRenderPassPtr m_CleanSceneColorPass;
	IKRenderPassPtr m_UnderBlendPass;
	IKRenderPassPtr m_PeelingPass;

	KShaderRef m_QuadVS;
	KShaderRef m_UnderBlendFS;
	
	IKPipelinePtr m_UnderBlendPeelingPipeline;
	IKPipelinePtr m_UnderBlendOpaquePipeline;

	IKSamplerPtr m_DeelingDepthSampler;

	uint32_t m_Width;
	uint32_t m_Height;
	uint32_t m_PeelingLayers;
public:
	KDepthPeeling();
	~KDepthPeeling();

	bool Init(uint32_t width, uint32_t height, uint32_t layers);
	bool UnInit();
	bool Resize(uint32_t width, uint32_t height);

	inline IKRenderTargetPtr GetPrevPeelingDepthTarget() { return m_PeelingDepthTarget[1]; }
	inline IKSamplerPtr GetPeelingDepthSampler() { return m_DeelingDepthSampler; }

	bool Execute(KRHICommandList& commandList, const std::vector<IKEntity*>& cullRes);
};