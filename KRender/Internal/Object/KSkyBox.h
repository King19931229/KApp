#pragma once

#include "Internal/KVertexDefinition.h"

class KSkyBox
{
protected:
	static KVertexDefinition::POS_3F_NORM_3F_UV_2F ms_Positions[8];
	static uint16_t ms_Indices[36];

	std::vector<IKPipelinePtr> m_Pipelines;
	std::vector<IKUniformBufferPtr> m_UniformBuffers;

	IKVertexBufferPtr m_VertexBuffer;
	IKIndexBufferPtr m_IndexBuffer;

	IKShaderPtr m_VertexShader;
	IKShaderPtr m_FragmentShader;

	IKTexturePtr m_CubeTexture;
	IKSamplerPtr m_CubeSampler;

	struct PushConstBlock
	{
		glm::mat4 model;
	} m_PushConstBlock;
	PushConstant m_Constant;
	PushConstantLocation m_ConstantLoc;

	void LoadResource(const char* cubeTexPath);
	void PreparePipeline();
public:
	KSkyBox();
	~KSkyBox();

	bool Init(IKRenderDevice* renderDevice,	size_t frameInFlight, const char* cubeTexPath);
	bool UnInit();

	bool Draw(unsigned int imageIndex, IKRenderTarget* target, void* commandBufferPtr);

	inline IKTexturePtr GetCubeTexture() { return m_CubeTexture; }
	inline IKSamplerPtr GetSampler() { return m_CubeSampler; }
};