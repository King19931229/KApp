#pragma once
#include "Interface/IKRenderScene.h"
#include "Interface/IKRenderConfig.h"
#include "Interface/IKTexture.h"
#include "Interface/IKRenderTarget.h"
#include "Interface/IKComputePipeline.h"
#include "Internal/KVertexDefinition.h"
#include "Internal/Object/KDebugDrawer.h"

class KClipmapVoxilzer;

struct KClipmapVoxelizationRegion
{
	glm::ivec3 min;
	glm::ivec3 max;

	KClipmapVoxelizationRegion()
	{
		min = glm::ivec3(0);
		max = glm::ivec3(0);
	}
	KClipmapVoxelizationRegion(const glm::ivec3& _min, const glm::ivec3& _max)
	{
		min = _min;
		max = _max;
	}
};

class KClipmapVoxilzerLevel
{
protected:
	KClipmapVoxilzer* m_Parent;
	KClipmapVoxelizationRegion m_Region;
	float m_VoxelSize;
	uint32_t m_LevelIdx;
	uint32_t m_MinUpdateChange;
	int32_t m_Extent;

	std::vector<KClipmapVoxelizationRegion> m_UpdateRegions;

	glm::mat4 m_ViewProjectionMatrix[3];
	glm::mat4 m_ViewProjectionMatrixI[3];
public:
	KClipmapVoxilzerLevel(KClipmapVoxilzer* parent, uint32_t level);
	~KClipmapVoxilzerLevel();

	void SetMinPosition(const glm::ivec3& min);
	inline void SetMinUpdateChange(uint32_t minChange) { m_MinUpdateChange = minChange; }

	const glm::ivec3& GetMin() const { return m_Region.min; }
	const glm::ivec3& GetMax() const { return  m_Region.max; }
	float GetVoxelSize() const { return m_VoxelSize; }

	uint32_t GetMinUpdateChange() const { return m_MinUpdateChange;	}
	uint32_t GetExtent() const { return m_Extent; }	

	const glm::vec3 WorldPositionToClipUVW(const glm::vec3& worldPos) const;
	const glm::vec3 ClipUVWToWorldPosition(const glm::vec3& uvw) const;

	const glm::ivec3 WorldPositionToClipCoord(const glm::vec3& worldPos) const;
	const glm::vec3 ClipCoordToWorldPosition(const glm::vec3& clipCoord) const;

	const glm::ivec3 ClipCoordToImagePosition(const glm::ivec3& clipCoord) const;
	const glm::ivec3 ImagePositionToClipCoord(const glm::ivec3& imagePos) const;

	const glm::vec3 GetWorldMin() const { return ClipCoordToWorldPosition(GetMin()); }
	const glm::vec3 GetWorldMax() const { return ClipCoordToWorldPosition(GetMax()); }
	float GetWorldExtent() const { return (float)m_Extent * GetVoxelSize(); }

	const glm::mat4* GetViewProjectionMatrix() const { return m_ViewProjectionMatrix; }
	const glm::mat4* GetViewProjectionMatrixInv() const { return m_ViewProjectionMatrixI; }

	void UpdateProjectionMatrices();

	void SetUpdateRegions(const std::vector<KClipmapVoxelizationRegion>& regions) { m_UpdateRegions = regions; }
	const std::vector<KClipmapVoxelizationRegion>& GetUpdateRegions() const { return m_UpdateRegions; }
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
	uint32_t m_ClipmapVolumeDimensionX;
	uint32_t m_ClipmapVolumeDimensionY;
	uint32_t m_ClipmapVolumeDimensionZ;
	uint32_t m_BorderSize;
	uint32_t m_ClipLevelCount;
	float m_BaseVoxelSize;

	IKCommandBufferPtr m_PrimaryCommandBuffer;
	IKCommandBufferPtr m_DrawCommandBuffer;
	IKCommandBufferPtr m_LightingCommandBuffer;
	IKCommandPoolPtr m_CommandPool;

	IKRenderTargetPtr m_VoxelRenderPassTarget;
	IKRenderPassPtr m_VoxelRenderPass;

	IKRenderTargetPtr m_LightPassTarget;
	IKRenderPassPtr m_LightPassRenderPass;

	std::vector<KClipmapVoxilzerLevel> m_ClipLevels;
	EntityObserverFunc m_OnSceneChangedFunc;

	bool m_VoxelBorderEnable;
	bool m_VoxelDebugUpdate;
	bool m_VoxelNeedUpdate;
	bool m_VoxelEmpty;

	void OnSceneChanged(EntitySceneOp op, IKEntityPtr entity);
	void SetupVoxelReleatedData();
	void SetupVoxelBuffer();

	glm::ivec3 ComputeMovementByCamera(uint32_t levelIdx);
	std::vector<KClipmapVoxelizationRegion> ComputeRevoxelizationRegionsByMovement(uint32_t levelIdx, const glm::ivec3& movement);

	void UpdateVoxelBuffer();
	void UpdateInternal();
	void VoxelizeStaticScene(IKCommandBufferPtr commandBuffer);
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