#pragma once
#include "Interface/IKRenderScene.h"
#include "Interface/IKRenderConfig.h"
#include "Interface/IKTexture.h"
#include "Interface/IKRenderTarget.h"
#include "Interface/IKComputePipeline.h"
#include "Internal/KVertexDefinition.h"
#include "Internal/Object/KDebugDrawer.h"
#include "Internal/Render/KRHICommandList.h"

enum VoxelClipmapBinding
{
	VOXEL_CLIPMAP_BINDING_STATIC_CSM0 = SHADER_BINDING_TEXTURE0,
	VOXEL_CLIPMAP_BINDING_STATIC_CSM1,
	VOXEL_CLIPMAP_BINDING_STATIC_CSM2,
	VOXEL_CLIPMAP_BINDING_STATIC_CSM3,

	VOXEL_CLIPMAP_BINDING_DYNAMIC_CSM0,
	VOXEL_CLIPMAP_BINDING_DYNAMIC_CSM1,
	VOXEL_CLIPMAP_BINDING_DYNAMIC_CSM2,
	VOXEL_CLIPMAP_BINDING_DYNAMIC_CSM3,

	VOXEL_CLIPMAP_BINDING_ALBEDO,
	VOXEL_CLIPMAP_BINDING_NORMAL,
	VOXEL_CLIPMAP_BINDING_EMISSION,
	VOXEL_CLIPMAP_BINDING_STATIC_FLAG,
	VOXEL_CLIPMAP_BINDING_DIFFUSE_MAP,
	VOXEL_CLIPMAP_BINDING_OPACITY_MAP,
	VOXEL_CLIPMAP_BINDING_EMISSION_MAP,
	VOXEL_CLIPMAP_BINDING_RADIANCE,
	VOXEL_CLIPMAP_BINDING_VISIBILITY,
	VOXEL_CLIPMAP_BINDING_RADIANCE2,

	VOXEL_CLIPMAP_BINDING_GBUFFER_RT0,
	VOXEL_CLIPMAP_BINDING_GBUFFER_RT1,
	VOXEL_CLIPMAP_BINDING_GBUFFER_RT2,
	VOXEL_CLIPMAP_BINDING_GBUFFER_RT3
};
static_assert(VOXEL_CLIPMAP_BINDING_GBUFFER_RT3 == SHADER_BINDING_TEXTURE21, "must match");

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
	uint32_t m_UpdateFrameNum;
	int32_t m_Extent;

	glm::ivec3 m_Movement;

	std::vector<KClipmapVoxelizationRegion> m_UpdateRegions;
	bool m_ForceUpdate;
	bool m_NeedUpdate;

	glm::mat4 m_ViewProjectionMatrix[3];
	glm::mat4 m_ViewProjectionMatrixI[3];
public:
	KClipmapVoxilzerLevel(KClipmapVoxilzer* parent, uint32_t level);
	~KClipmapVoxilzerLevel();

	void SetUpdateMovement(const glm::ivec3& movement);
	void ApplyUpdateMovement();

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

	void SetUpdateRegions(const std::vector<KClipmapVoxelizationRegion>& regions);
	void MarkUpdateFinish();
	void SetForceUpdate(bool forceUpdate);
	bool IsUpdateFrame() const;

	const std::vector<KClipmapVoxelizationRegion>& GetUpdateRegions() const { return m_UpdateRegions; }
	bool NeedUpdate() const { return m_NeedUpdate; }
};

class KClipmapVoxilzer
{
protected:
	enum
	{
		VOXEL_CLIPMAP_GROUP_SIZE = 8,
		VOXEL_CLIPMPA_MAX_LEVELCOUNT = 9
	};

	static const VertexFormat ms_VertexFormats[1];

	IKRenderScene* m_Scene;
	const KCamera* m_Camera;
	uint32_t m_Width;
	uint32_t m_Height;

	IKRenderTargetPtr m_StaticFlag;
	IKRenderTargetPtr m_VoxelAlbedo;
	IKRenderTargetPtr m_VoxelNormal;
	IKRenderTargetPtr m_VoxelEmissive;
	IKRenderTargetPtr m_VoxelRadiance;
	IKRenderTargetPtr m_VoxelVisibility;

	uint32_t m_VolumeDimension;
	uint32_t m_ClipmapVolumeDimensionX;
	uint32_t m_ClipmapVolumeDimensionY;
	uint32_t m_ClipmapVolumeDimensionZ;
	uint32_t m_ClipmapVolumeDimensionX6Face;
	uint32_t m_BorderSize;
	uint32_t m_ClipLevelCount;
	float m_BaseVoxelSize;
	float m_ScreenRatio;

	IKRenderTargetPtr m_VoxelRenderPassTarget;
	IKRenderPassPtr m_VoxelRenderPass;

	IKRenderTargetPtr m_LightPassTarget;
	IKRenderTargetPtr m_LightComposeTarget;
	IKRenderPassPtr m_LightPassRenderPass;
	IKRenderPassPtr m_LightComposeRenderPass;

	KShaderRef m_VoxelDrawVS;
	KShaderRef m_VoxelDrawGS;
	KShaderRef m_VoxelWireFrameDrawGS;
	KShaderRef m_VoxelDrawFS;

	IKPipelinePtr m_VoxelDrawPipeline;
	IKPipelinePtr m_VoxelWireFrameDrawPipeline;

	IKComputePipelinePtr m_ClearRegionPipeline;
	IKComputePipelinePtr m_ClearRadiancePipeline;
	IKComputePipelinePtr m_InjectRadiancePipeline;
	IKComputePipelinePtr m_InjectPropagationPipeline;

	IKComputePipelinePtr m_DownSampleVisibilityPipeline;
	IKComputePipelinePtr m_DownSampleRadiancePipeline;
	IKComputePipelinePtr m_WrapRadianceBorderPipeline;

	KShaderRef m_QuadVS;
	KShaderRef m_LightPassFS;
	KShaderRef m_LightComposeFS;
	IKPipelinePtr m_LightPassPipeline;
	IKPipelinePtr m_LightComposePipeline;

	IKSamplerPtr m_CloestSampler;
	IKSamplerPtr m_LinearSampler;

	KVertexData m_VoxelDrawVertexData;

	std::vector<KClipmapVoxilzerLevel> m_ClipLevels;

	EntityObserverFunc m_OnSceneChangedFunc;

	bool m_Enable;

	bool m_InjectFirstBounce;

	bool m_VoxelDrawEnable;
	bool m_VoxelDrawWireFrame;

	bool m_VoxelBorderEnable;
	bool m_VoxelDebugUpdate;
	bool m_VoxelEmpty;
	bool m_VoxelDebugVoxelize;

	float m_VoxelDrawBias;

	KRTDebugDrawer m_LightDebugDrawer;

	void OnSceneChanged(EntitySceneOp op, IKEntity* entity);
	void SetupVoxelReleatedData();
	void SetupVoxelBuffer();
	void SetupVoxelPipeline();
	void SetupVoxelDrawPipeline();
	void SetupLightPassPipeline();

	glm::ivec3 ComputeMovementByCamera(uint32_t levelIdx);
	std::vector<KClipmapVoxelizationRegion> ComputeRevoxelizationRegionsByMovement(uint32_t levelIdx, const glm::ivec3& movement);

	void UpdateVoxelBuffer(KRHICommandList& commandList);
	void UpdateInternal(KRHICommandList& commandList);
	void ApplyUpdateMovement();
	void ClearUpdateRegion(KRHICommandList& commandList);
	void VoxelizeStaticScene(KRHICommandList& commandList);
	void ClearRadiance(KRHICommandList& commandList);
	void UpdateRadiance(KRHICommandList& commandList);
	void InjectRadiance(KRHICommandList& commandList);
	void InjectPropagation(KRHICommandList& commandList);
	void DownSampleVisibility(KRHICommandList& commandList);
	void DownSampleRadiance(KRHICommandList& commandList);
	void WrapBorder(KRHICommandList& commandList);

	bool UpdateLightingResult(KRHICommandList& commandList);
public:
	KClipmapVoxilzer();
	~KClipmapVoxilzer();

	void UpdateVoxel(KRHICommandList& commandList);
	void Reload();

	inline uint32_t GetVoxelDimension() const { return m_VolumeDimension; }
	inline float GetBaseVoxelSize() const { return m_BaseVoxelSize; }

	void Resize(uint32_t width, uint32_t height);
	bool RenderVoxel(IKRenderPassPtr renderPass, KRHICommandList& commandList);
	bool UpdateFrame(KRHICommandList& commandList);

	bool& GetEnable() { return m_Enable; }
	bool& GetVoxelDebugUpdate() { return m_VoxelDebugUpdate; }
	bool& GetVoxelDebugVoxelize() { return m_VoxelDebugVoxelize; }
	bool& GetVoxelDrawEnable() { return m_VoxelDrawEnable; }
	bool& GetVoxelDrawWireFrame() { return m_VoxelDrawWireFrame; }
	bool& GetLightDebugDrawEnable() { return m_LightDebugDrawer.GetEnable(); }

	float& GetVoxelDrawBias() { return m_VoxelDrawBias; }

	inline bool IsVoxelDrawEnable() const { return m_VoxelDrawEnable; }
	inline void SetVoxelDrawEnable(bool enable) { m_VoxelDrawEnable = enable; }

	inline bool IsVoxelDrawWireFrame() const { return m_VoxelDrawWireFrame; }
	inline void SetVoxelDrawWireFrame(bool wireframe) { m_VoxelDrawWireFrame = wireframe; }

	bool DebugRender(IKRenderPassPtr renderPass, KRHICommandList& commandList);

	IKFrameBufferPtr GetStaticFlag() { return m_StaticFlag ? m_StaticFlag->GetFrameBuffer() : nullptr; }
	IKFrameBufferPtr GetVoxelAlbedo() { return m_VoxelAlbedo ? m_VoxelAlbedo->GetFrameBuffer() : nullptr; }
	IKFrameBufferPtr GetVoxelNormal() { return m_VoxelNormal ? m_VoxelNormal->GetFrameBuffer() : nullptr; }
	IKFrameBufferPtr GetVoxelEmissive() { return m_VoxelEmissive ? m_VoxelEmissive->GetFrameBuffer() : nullptr; }
	IKFrameBufferPtr GetVoxelRadiance() { return m_VoxelRadiance ? m_VoxelRadiance->GetFrameBuffer() : nullptr; }
	IKFrameBufferPtr GetVoxelVisibility() { return m_VoxelVisibility ? m_VoxelVisibility->GetFrameBuffer() : nullptr; }

	bool Init(IKRenderScene* scene, const KCamera* camera, uint32_t dimension, uint32_t levelCount,	uint32_t baseVoxelSize, uint32_t width, uint32_t height, float ratio);
	bool UnInit();

	inline IKRenderTargetPtr GetFinalMask() { return m_LightComposeTarget; }
};