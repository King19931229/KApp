#pragma once

#include "Internal/KVertexDefinition.h"

class KSkyBox
{
protected:
	static KVertexDefinition::POS_3F_NORM_3F_UV_2F ms_Positions[8];
	static uint16_t ms_Indices[36];

	std::vector<IKPipelinePtr> m_Pipelines;

	IKVertexBufferPtr m_VertexBuffer;
	IKIndexBufferPtr m_IndexBuffer;
	IKUniformBufferPtr m_TransformBuffer;

	IKShaderPtr m_VertexShader;
	IKShaderPtr m_FragmentShader;

	IKTexturePtr m_CubeTexture;
	IKSamplerPtr m_CubeSampler;

public:
	KSkyBox();
	~KSkyBox();

	bool Init(IKRenderDevice* renderDevice,	const std::vector<IKRenderTarget*>& renderTargets,
		IKTexturePtr cubeTexture);
	bool UnInit();

	bool Draw(unsigned int imageIndex, void* commandBufferPtr, const glm::mat4& viewProj);
};