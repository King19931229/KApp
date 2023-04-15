#pragma once

#include "Interface/IKRenderDevice.h"
#include "Interface/IKRenderTarget.h"
#include "Interface/IKRenderPass.h"
#include "Internal/KVertexDefinition.h"
#include "Internal/ShaderMap/KShaderMap.h"

class KPrefilerCubeMap
{
protected:
	static const KVertexDefinition::SCREENQUAD_POS_2F ms_Vertices[4];
	static const uint32_t ms_Indices[6];

	enum
	{
		SH_BINDING_COEFFICIENT = 0,
		SH_BINDING_CUBEMAP = 1,
		SH_GROUP_SIZE = 16
	};

	// 预处理相关资源
	struct ConstantBlock
	{
		glm::vec4 up;
		glm::vec4 right;
		glm::vec4 center;
		glm::vec4 roughness;
	};

	KTextureBinding m_TextureBinding;
	struct MipmapTarget
	{
		IKRenderTargetPtr target;
		IKRenderPassPtr	pass;
	};
	std::vector<MipmapTarget> m_MipmapTargets;
	IKPipelinePtr m_DiffuseIrradiancePipeline;
	IKPipelinePtr m_SpecularIrradiancePipeline;
	IKPipelinePtr m_IntegrateBRDFPipeline;

	IKComputePipelinePtr m_SHProductPipeline;
	IKComputePipelinePtr m_SHConstructPipeline;
	IKStorageBufferPtr m_SHCoffBuffer;

	IKVertexBufferPtr m_VertexBuffer;
	IKIndexBufferPtr m_IndexBuffer;

	KShaderMap m_DiffuseIrradianceShaderMap;
	KShaderMap m_SpecularIrradianceShaderMap;
	KShaderMap m_IntegrateBRDFShaderMap;

	// Texture
	IKTexturePtr m_SrcCubeMap;
	IKSamplerPtr m_SrcCubeSampler;

	IKTexturePtr m_DiffuseIrradianceMap;
	IKSamplerPtr m_DiffuseIrradianceSampler;

	IKTexturePtr m_SpecularIrradianceMap;
	IKSamplerPtr m_SpecularIrradianceSampler;

	IKTexturePtr m_SHConstructCubeMap;

	IKRenderTargetPtr m_IntegrateBRDFTarget;
	IKRenderPassPtr m_IntegrateBRDFPass;
	IKSamplerPtr m_IntegrateBRDFSampler;

	glm::vec4 m_SHCoeff[9];

	bool PopulateCubeMapRenderCommand(KRenderCommand& command, uint32_t faceIndex, float roughtness, IKPipelinePtr pipeline, IKRenderPassPtr renderPass);
	bool AllocateTempResource(IKRenderDevice* renderDevice, uint32_t width, uint32_t height, size_t mipmaps);
	bool FreeTempResource();
	bool Compute();
public:
	KPrefilerCubeMap();
	~KPrefilerCubeMap();

	bool Init(uint32_t width, uint32_t height, size_t mipmaps, const char* cubemapPath);
	bool UnInit();

	IKTexturePtr GetDiffuseIrradiance() { return m_DiffuseIrradianceMap; }
	IKTexturePtr GetSpecularIrradiance() { return m_SpecularIrradianceMap; }
	IKRenderTargetPtr GetIntegrateBRDF() { return m_IntegrateBRDFTarget; }

	IKSamplerPtr GetDiffuseIrradianceSampler() { return m_DiffuseIrradianceSampler; }
	IKSamplerPtr GetSpecularIrradianceSampler() { return m_SpecularIrradianceSampler; }
	IKSamplerPtr GetIntegrateBRDFSampler() { return m_IntegrateBRDFSampler; }
};