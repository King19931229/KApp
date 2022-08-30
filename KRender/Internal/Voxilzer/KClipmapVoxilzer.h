#pragma once
#include "Interface/IKRenderScene.h"
#include "Interface/IKRenderConfig.h"
#include "Interface/IKTexture.h"
#include "Interface/IKRenderTarget.h"
#include "Interface/IKComputePipeline.h"
#include "Internal/KVertexDefinition.h"
#include "Internal/Object/KDebugDrawer.h"

class KClipmapVoxilzer;

class KClipmapVoxilzerLevel
{
protected:
	KClipmapVoxilzer* m_Parent;
	uint32_t m_LevelIdx;
	float m_VoxelSize;
	glm::ivec3 m_Min;
	glm::ivec3 m_Max;
	int32_t m_Extent;

	glm::mat4 m_ViewProjectionMatrix[3];
	glm::mat4 m_ViewProjectionMatrixI[3];
public:
	KClipmapVoxilzerLevel(KClipmapVoxilzer* parent, uint32_t level);
	~KClipmapVoxilzerLevel();

	void SetPosition(const glm::ivec3& min);

	const glm::ivec3& GetMin() const { return m_Min; }
	const glm::ivec3& GetMax() const { return m_Max; }
	uint32_t GetExtent() const { return m_Extent; }

	float GetVoxelSize() const { return m_VoxelSize; }

	const glm::vec3 WorldPositionToClipUVW(const glm::vec3& worldPos) const;
	const glm::vec3 ClipUVWToWorldPosition(const glm::vec3& uvw) const;

	const glm::ivec3 WorldPositionToClipCoord(const glm::vec3& worldPos) const;
	const glm::vec3 ClipCoordToWorldPosition(const glm::vec3& clipCoord) const;

	const glm::ivec3 ClipCoordToImagePosition(const glm::ivec3& clipCoord) const;
	const glm::ivec3 ImagePositionToClipCoord(const glm::ivec3& imagePos) const;

	const glm::vec3 GetWorldMin() const { return ClipCoordToWorldPosition(m_Min); }
	const glm::vec3 GetWorldMax() const { return ClipCoordToWorldPosition(m_Max); }
	float GetWorldExtent() const { return (float)m_Extent * m_VoxelSize; }

	const glm::mat4* GetViewProjectionMatrix() const { return m_ViewProjectionMatrix; }
	const glm::mat4* GetViewProjectionMatrixInv() const { return m_ViewProjectionMatrixI; }

	void UpdateProjectionMatrices();
};

class KClipmapVoxilzer
{
protected:
	IKRenderScene* m_Scene;
	const KCamera* m_Camera;
	uint32_t m_Width;
	uint32_t m_Height;

	IKRenderTargetPtr m_StaticFlag;
	IKRenderTargetPtr m_VoxelAlbedo;
	IKRenderTargetPtr m_VoxelNormal;
	IKRenderTargetPtr m_VoxelEmissive;
	IKRenderTargetPtr m_VoxelRadiance;

	uint32_t m_VolumeDimension;
	uint32_t m_BorderSize;
	uint32_t m_ClipLevelCount;
	float m_BaseVoxelSize;

	IKCommandBufferPtr m_PrimaryCommandBuffer;
	IKCommandBufferPtr m_DrawCommandBuffer;
	IKCommandBufferPtr m_LightingCommandBuffer;
	IKCommandPoolPtr m_CommandPool;

	IKRenderTargetPtr m_VoxelRenderPassTarget;
	IKRenderPassPtr m_VoxelRenderPass;

	std::vector<KClipmapVoxilzerLevel> m_ClipLevels;
	EntityObserverFunc m_OnSceneChangedFunc;

	bool m_VoxelBorderEnable;
	bool m_VoxelDebugUpdate;
	bool m_VoxelNeedUpdate;

	void OnSceneChanged(EntitySceneOp op, IKEntityPtr entity);
	void SetupVoxelBuffer();

	void UpdateInternal();
public:
	KClipmapVoxilzer();
	~KClipmapVoxilzer();

	void UpdateVoxel();

	inline uint32_t GetVoxelDimension() const { return m_VolumeDimension; }
	inline float GetBaseVoxelSize() const { return m_BaseVoxelSize; }

	void Resize(uint32_t width, uint32_t height);

	IKFrameBufferPtr GetStaticFlag() { return m_StaticFlag ? m_StaticFlag->GetFrameBuffer() : nullptr; }
	IKFrameBufferPtr GetVoxelAlbedo() { return m_VoxelAlbedo ? m_VoxelAlbedo->GetFrameBuffer() : nullptr; }
	IKFrameBufferPtr GetVoxelNormal() { return m_VoxelNormal ? m_VoxelNormal->GetFrameBuffer() : nullptr; }
	IKFrameBufferPtr GetVoxelEmissive() { return m_VoxelEmissive ? m_VoxelEmissive->GetFrameBuffer() : nullptr; }
	IKFrameBufferPtr GetVoxelRadiance() { return m_VoxelRadiance ? m_VoxelRadiance->GetFrameBuffer() : nullptr; }

	bool Init(IKRenderScene* scene, const KCamera* camera, uint32_t dimension, uint32_t levelCount,	uint32_t width, uint32_t height);
	bool UnInit();
};