#pragma once

#include "Interface/IKRenderDevice.h"
#include "Internal/KVertexDefinition.h"

class KSkyBox
{
protected:
	static const KVertexDefinition::POS_3F_NORM_3F_UV_2F ms_Positions[8];
	static const uint16_t ms_Indices[36];
	static const VertexFormat ms_VertexFormats[1];

	std::vector<IKPipelinePtr> m_Pipelines;

	std::vector<IKCommandBufferPtr> m_CommandBuffers;
	IKCommandPoolPtr m_CommandPool;

	IKVertexBufferPtr m_VertexBuffer;
	IKIndexBufferPtr m_IndexBuffer;

	IKShaderPtr m_VertexShader;
	IKShaderPtr m_FragmentShader;

	IKTexturePtr m_CubeTexture;
	IKSamplerPtr m_CubeSampler;

	KVertexData m_VertexData;
	KIndexData m_IndexData;

	void LoadResource(const char* cubeTexPath);
	void PreparePipeline();
	void InitRenderData();
public:
	KSkyBox();
	~KSkyBox();

	bool Init(IKRenderDevice* renderDevice,	size_t frameInFlight, const char* cubeTexPath);
	bool UnInit();

	bool Render(size_t frameIndex, IKRenderTargetPtr target, std::vector<IKCommandBufferPtr>& buffers);

	inline IKTexturePtr GetCubeTexture() { return m_CubeTexture; }
	inline IKSamplerPtr GetSampler() { return m_CubeSampler; }
};