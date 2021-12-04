#pragma once
#include "Interface/IKRenderScene.h"
#include "Interface/IKRenderConfig.h"
#include "Interface/IKTexture.h"
#include "Interface/IKRenderTarget.h"
#include "Interface/IKComputePipeline.h"
#include "Internal/KVertexDefinition.h"

class KVoxilzer
{
protected:
	enum
	{
		GROUP_SIZE = 8
	};

	static const VertexFormat ms_VertexFormats[1];

	static const KVertexDefinition::SCREENQUAD_POS_2F ms_QuadVertices[4];
	static const uint16_t ms_QuadIndices[6];
	static const VertexFormat ms_QuadFormats[1];

	IKVertexBufferPtr m_QuadVertexBuffer;
	IKIndexBufferPtr m_QuadIndexBuffer;

	KVertexData m_QuadVertexData;
	KIndexData m_QuadIndexData;

	IKRenderScene* m_Scene;

	IKRenderTargetPtr m_StaticFlag;
	IKRenderTargetPtr m_VoxelAlbedo;
	IKRenderTargetPtr m_VoxelNormal;
	IKRenderTargetPtr m_VoxelEmissive;
	IKRenderTargetPtr m_VoxelRadiance;
	IKRenderTargetPtr m_VoxelTexMipmap[6];

	IKRenderTargetPtr m_LightPassTarget;

	IKSamplerPtr m_CloestSampler;
	IKSamplerPtr m_LinearSampler;
	IKSamplerPtr m_MipmapSampler;

	uint32_t m_VolumeDimension;
	uint32_t m_VoxelCount;
	uint32_t m_NumMipmap;
	float m_VolumeGridSize;
	float m_VoxelSize;

	glm::mat4 m_ViewProjectionMatrix[3];
	glm::mat4 m_ViewProjectionMatrixI[3];

	IKCommandBufferPtr m_PrimaryCommandBuffer;
	std::vector<IKCommandBufferPtr> m_CommandBuffers;
	IKCommandPoolPtr m_CommandPool;
	IKRenderTargetPtr m_RenderPassTarget;
	IKRenderPassPtr m_RenderPass;

	IKShaderPtr m_VoxelDrawVS;
	IKShaderPtr m_VoxelDrawGS;
	IKShaderPtr m_VoxelDrawFS;

	std::vector<IKPipelinePtr> m_VoxelDrawPipelines;

	IKComputePipelinePtr m_InjectRadiancePipeline;
	IKComputePipelinePtr m_InjectPropagationPipeline;

	IKComputePipelinePtr m_MipmapBasePipeline;
	IKComputePipelinePtr m_MipmapVolumePipeline;

	IKShaderPtr m_LightPassVS;
	IKShaderPtr m_LightPassFS;
	std::vector<IKPipelinePtr> m_LightPassPipelines;

	KVertexData m_VoxelDrawVertexData;

	EntityObserverFunc m_OnSceneChangedFunc;

	bool m_InjectFirstBounce;
	bool m_VoxelDrawEnable;
	bool m_VoxelDebugUpdate;

	void OnSceneChanged(EntitySceneOp op, IKEntityPtr entity);
	void UpdateProjectionMatrices();
	void SetupVoxelVolumes(uint32_t dimension);
	void SetupVoxelDrawPipeline();
	void SetupRadiancePipeline();
	void SetupMipmapPipeline();
	void SetupLightPassPipeline();

	void VoxelizeStaticScene(IKCommandBufferPtr commandBuffer);
	void UpdateRadiance(IKCommandBufferPtr commandBuffer);
	void InjectRadiance(IKCommandBufferPtr commandBuffer);
	void GenerateMipmap(IKCommandBufferPtr commandBuffer);
	void GenerateMipmapBase(IKCommandBufferPtr commandBuffer);
	void GenerateMipmapVolume(IKCommandBufferPtr commandBuffer);

	void UpdateInternal();
public:
	KVoxilzer();
	~KVoxilzer();

	void Update();
	void ReloadShader();

	inline bool IsVoxelDrawEnable() const { return m_VoxelDrawEnable; }
	inline void SetVoxelDrawEnable(bool enable) { m_VoxelDrawEnable = enable; }

	bool RenderVoxel(size_t frameIndex, IKRenderPassPtr renderPass, std::vector<IKCommandBufferPtr>& buffers);

	IKFrameBufferPtr GetStaticFlag() { return m_StaticFlag ? m_StaticFlag->GetFrameBuffer() : nullptr; }
	IKFrameBufferPtr GetVoxelAlbedo() { return m_VoxelAlbedo ? m_VoxelAlbedo->GetFrameBuffer() : nullptr; }
	IKFrameBufferPtr GetVoxelNormal() { return m_VoxelNormal ? m_VoxelNormal->GetFrameBuffer() : nullptr; }
	IKFrameBufferPtr GetVoxelEmissive() { return m_VoxelEmissive ? m_VoxelEmissive->GetFrameBuffer() : nullptr; }
	IKFrameBufferPtr GetVoxelRadiance() { return m_VoxelRadiance ? m_VoxelRadiance->GetFrameBuffer() : nullptr; }
	IKFrameBufferPtr GetVoxelTexMipmap(uint32_t face) { return (face < 6 && m_VoxelTexMipmap[face]) ? m_VoxelTexMipmap[face]->GetFrameBuffer() : nullptr; }

	bool Init(IKRenderScene* scene, uint32_t dimension);
	bool UnInit();
};