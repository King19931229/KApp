#pragma once
#include "Interface/IKRenderScene.h"
#include "Interface/IKRenderConfig.h"
#include "Interface/IKTexture.h"

class KVoxilzer
{
protected:
	IKRenderScene* m_Scene;

	IKTexturePtr m_StaticFlag;
	IKTexturePtr m_VoxelAlbedo;
	IKTexturePtr m_VoxelNormal;
	IKTexturePtr m_VoxelEmissive;
	IKTexturePtr m_VoxelRadiance;
	IKTexturePtr m_VoxelTexMipmap[6];

	IKSamplerPtr m_CloestSampler;
	IKSamplerPtr m_LinearSampler;
	IKSamplerPtr m_MipmapSampler;

	uint32_t m_VolumeDimension;
	uint32_t m_VoxelCount;
	float m_VolumeGridSize;
	float m_VoxelSize;

	glm::mat4 m_ViewProjectionMatrix[3];
	glm::mat4 m_ViewProjectionMatrixI[3];

	IKCommandBufferPtr m_CommandBuffer;
	IKCommandPoolPtr m_CommandPool;
	IKRenderTargetPtr m_RenderPassTarget;
	IKRenderPassPtr m_RenderPass;

	EntityObserverFunc m_OnSceneChangedFunc;

	void OnSceneChanged(EntitySceneOp op, IKEntityPtr entity);
	void UpdateProjectionMatrices();
	void SetupVoxelVolumes(uint32_t dimension);
	void VoxelizeStaticScene();
public:
	KVoxilzer();
	~KVoxilzer();

	IKFrameBufferPtr GetStaticFlag() { return m_StaticFlag ? m_StaticFlag->GetFrameBuffer() : nullptr; }
	IKFrameBufferPtr GetVoxelAlbedo() { return m_VoxelAlbedo ? m_VoxelAlbedo->GetFrameBuffer() : nullptr; }
	IKFrameBufferPtr GetVoxelNormal() { return m_VoxelNormal ? m_VoxelNormal->GetFrameBuffer() : nullptr; }
	IKFrameBufferPtr GetVoxelEmissive() { return m_VoxelEmissive ? m_VoxelEmissive->GetFrameBuffer() : nullptr; }
	IKFrameBufferPtr GetVoxelRadiance() { return m_VoxelRadiance ? m_VoxelRadiance->GetFrameBuffer() : nullptr; }
	IKFrameBufferPtr GetVoxelTexMipmap(uint32_t mipmap) { return (mipmap < 6 && m_VoxelTexMipmap[mipmap]) ? m_VoxelTexMipmap[mipmap]->GetFrameBuffer() : nullptr; }

	bool Init(IKRenderScene* scene, uint32_t dimension);
	bool UnInit();
};