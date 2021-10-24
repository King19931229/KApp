#pragma once
#include "Interface/IKRenderScene.h"
#include "Interface/IKRenderConfig.h"

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

	bool Init(IKRenderScene* scene, uint32_t dimension);
	bool UnInit();
};