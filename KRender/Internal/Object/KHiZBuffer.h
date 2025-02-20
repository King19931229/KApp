#pragma once
#include "Interface/IKRenderDevice.h"
#include "Interface/IKShader.h"
#include "Interface/IKSampler.h"
#include "Internal/Render/KRHICommandList.h"

class KHiZBuffer
{
protected:
	IKRenderTargetPtr m_HiZMinBuffer;
	IKRenderTargetPtr m_HiZMaxBuffer;
	uint32_t m_NumMips;
	uint32_t m_HiZWidth;
	uint32_t m_HiZHeight;
	KShaderRef m_QuadVS;
	KShaderRef m_ReadDepthFS;
	KShaderRef m_BuildHiZFS;
	KSamplerRef m_ReadDepthSampler;
	KSamplerRef m_HiZBuildSampler;

	KSamplerRef m_HiZSampler;

	IKPipelinePtr m_ReadDepthPipeline;
	std::vector<IKPipelinePtr> m_BuildHiZMinPipelines;
	std::vector<IKPipelinePtr> m_BuildHiZMaxPipelines;
	std::vector<IKRenderPassPtr> m_HiZMinRenderPass;
	std::vector<IKRenderPassPtr> m_HiZMaxRenderPass;

	void InitializePipeline();
public:
	KHiZBuffer();
	~KHiZBuffer();

	bool Init(uint32_t width, uint32_t height);
	bool UnInit();
	bool Resize(uint32_t width, uint32_t height);

	bool Reload();

	IKSamplerPtr GetHiZSampler() { return *m_HiZSampler; }
	IKRenderTargetPtr GetMinBuffer() { return m_HiZMinBuffer; }
	IKRenderTargetPtr GetMaxBuffer() { return m_HiZMaxBuffer; }

	uint32_t GetNumMips() const { return m_NumMips; }

	bool Construct(KRHICommandList& commandList);
};