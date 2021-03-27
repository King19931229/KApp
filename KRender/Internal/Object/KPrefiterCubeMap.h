#pragma once

#include "Interface/IKRenderDevice.h"
#include "Interface/IKMaterial.h"
#include "Interface/IKRenderTarget.h"
#include "Interface/IKRenderPass.h"
#include "Internal/KVertexDefinition.h"
#include "Internal/Asset/Material/KMaterialTextureBinding.h"

class KPrefilerCubeMap
{
protected:
	static const KVertexDefinition::SCREENQUAD_POS_2F ms_Vertices[4];
	static const uint32_t ms_Indices[6];

	// 预处理相关资源
	struct ConstantBlock
	{
		glm::vec4 up;
		glm::vec4 right;
		glm::vec4 center;
		glm::vec4 roughness;
	};

	KMaterialTextureBinding m_TextureBinding;
	struct MipmapTarget
	{
		IKRenderTargetPtr target;
		IKRenderPassPtr	pass;
	};
	std::vector<MipmapTarget> m_MipmapTargets;
	IKPipelinePtr m_DiffuseIrradiancePipeline;
	IKPipelinePtr m_SpecularIrradiancePipeline;
	IKPipelinePtr m_IntegrateBRDFPipeline;
	IKCommandBufferPtr m_CommandBuffer;
	IKCommandPoolPtr m_CommandPool;

	IKVertexBufferPtr m_VertexBuffer;
	IKIndexBufferPtr m_IndexBuffer;

	KVertexData m_SharedVertexData;
	KIndexData m_SharedIndexData;

	IKVertexBufferPtr m_SharedVertexBuffer;
	IKIndexBufferPtr m_SharedIndexBuffer;

	IKMaterialPtr m_DiffuseIrradianceMaterial;
	IKMaterialPtr m_SpecularIrradianceMaterial;
	IKMaterialPtr m_IntegrateBRDFMaterial;

	// Texture
	IKTexturePtr m_SrcCubeMap;
	IKSamplerPtr m_SrcCubeSampler;

	IKTexturePtr m_DiffuseIrradianceMap;
	IKSamplerPtr m_DiffuseIrradianceSampler;

	IKTexturePtr m_SpecularIrradianceMap;
	IKSamplerPtr m_SpecularIrradianceSampler;

	IKRenderTargetPtr m_IntegrateBRDFTarget;
	IKRenderPassPtr m_IntegrateBRDFPass;
	IKSamplerPtr m_IntegrateBRDFSampler;

	bool PopulateCubeMapRenderCommand(KRenderCommand& command, uint32_t faceIndex, float roughtness, IKPipelinePtr pipeline, IKRenderPassPtr renderPass);
	bool AllocateTempResource(IKRenderDevice* renderDevice,
		uint32_t width, uint32_t height,
		size_t mipmaps,
		const char* diffuseIrradiance,
		const char* specularIrradiance,
		const char* integrateBRDF);
	bool FreeTempResource();
	bool Draw();
public:
	KPrefilerCubeMap();
	~KPrefilerCubeMap();

	bool Init(IKRenderDevice* renderDevice,
		size_t frameInFlight,
		uint32_t width, uint32_t height,
		size_t mipmaps,
		const char* cubemapPath,
		const char* diffuseIrradiance,
		const char* specularIrradiance,
		const char* integrateBRDF);
	bool UnInit();

	IKTexturePtr GetDiffuseIrradiance() { return m_DiffuseIrradianceMap; }
	IKTexturePtr GetSpecularIrradiance() { return m_SpecularIrradianceMap; }
	IKRenderTargetPtr GetIntegrateBRDF() { return m_IntegrateBRDFTarget; }

	IKSamplerPtr GetDiffuseIrradianceSampler() { return m_DiffuseIrradianceSampler; }
	IKSamplerPtr GetSpecularIrradianceSampler() { return m_SpecularIrradianceSampler; }
	IKSamplerPtr GetIntegrateBRDFSampler() { return m_IntegrateBRDFSampler; }
};