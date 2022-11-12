#pragma once

#include "Interface/IKRenderDevice.h"
#include "Interface/IKTexture.h"
#include "Internal/KVertexDefinition.h"

class KSkyBox
{
protected:
	static const KVertexDefinition::POS_3F_NORM_3F_UV_2F ms_Positions[8];
	static const uint16_t ms_Indices[36];
	static const VertexFormat ms_VertexFormats[1];

	IKPipelinePtr m_Pipeline;

	IKCommandBufferPtr m_CommandBuffer;
	IKCommandPoolPtr m_CommandPool;

	IKVertexBufferPtr m_VertexBuffer;
	IKIndexBufferPtr m_IndexBuffer;

	KShaderRef m_VertexShader;
	KShaderRef m_FragmentShader;

	KTextureRef m_CubeTexture;
	IKSamplerPtr m_CubeSampler;

	KVertexData m_VertexData;
	KIndexData m_IndexData;

	void LoadResource(const char* cubeTexPath);
	void PreparePipeline();
	void InitRenderData();
public:
	KSkyBox();
	~KSkyBox();

	bool Init(IKRenderDevice* renderDevice,	const char* cubeTexPath);
	bool UnInit();

	bool Render(IKRenderPassPtr renderPass, IKCommandBufferPtr primaryBuffer);

	inline IKTexturePtr GetCubeTexture() { return *m_CubeTexture; }
	inline IKSamplerPtr GetSampler() { return m_CubeSampler; }
};