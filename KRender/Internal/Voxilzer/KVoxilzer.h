#pragma once
#include "Interface/IKRenderScene.h"
#include "Interface/IKRenderConfig.h"
#include "Interface/IKTexture.h"
#include "Interface/IKRenderTarget.h"
#include "Interface/IKComputePipeline.h"
#include "Internal/KVertexDefinition.h"
#include "Internal/Object/KDebugDrawer.h"

enum VoxelBinding
{
	VOXEL_BINDING_ALBEDO = SHADER_BINDING_TEXTURE0,
	VOXEL_BINDING_COUNTER = VOXEL_BINDING_ALBEDO,

	VOXEL_BINDING_NORMAL,
	VOXEL_BINDING_FRAGMENTLIST = VOXEL_BINDING_NORMAL,

	VOXEL_BINDING_EMISSION,
	VOXEL_BINDING_COUNTONLY = VOXEL_BINDING_EMISSION,

	VOXEL_BINDING_STATIC_FLAG,
	VOXEL_BINDING_DIFFUSE_MAP,
	VOXEL_BINDING_OPACITY_MAP,
	VOXEL_BINDING_EMISSION_MAP,
	VOXEL_BINDING_RADIANCE,
	VOXEL_BINDING_TEXMIPMAP_IN,
	VOXEL_BINDING_TEXMIPMAP_OUT,

	VOXEL_BINDING_GBUFFER_RT0,
	VOXEL_BINDING_GBUFFER_RT1,
	VOXEL_BINDING_GBUFFER_RT2,
	VOXEL_BINDING_GBUFFER_RT3,

	VOXEL_BINDING_OCTREE,
	VOXEL_BINDING_OCTREE_DATA,
	VOXEL_BINDING_OCTREE_MIPMAP_DATA
};
static_assert(VOXEL_BINDING_OCTREE_MIPMAP_DATA == SHADER_BINDING_TEXTURE16, "must match");

class KVoxilzer
{
protected:
	enum
	{
		VOXEL_GROUP_SIZE = 8,
		OCTREE_LEVEL_MIN = 1,
		OCTREE_LEVEL_MAX = 12,
		OCTREE_NODE_NUM_MIN = 1000000,
		OCTREE_NODE_NUM_MAX = 500000000,
		OCTREE_NODE_SIZE = sizeof(uint32_t),
		OCTREE_DATA_SIZE = sizeof(uint32_t) * 4,
		OCTREE_MIPMAP_DATA_SIZE = sizeof(uint32_t) * 6
	};

	enum OctreeBuildBinding
	{
		OCTREE_BINDING_COUNTER,
		OCTREE_BINDING_OCTREE,
		OCTREE_BINDING_OCTREE_DATA,
		OCTREE_BINDING_OCTREE_MIPMAP_DATA,
		OCTREE_BINDING_FRAGMENTLIST,
		OCTREE_BINDING_BUILDINFO,
		OCTREE_BINDING_INDIRECT,
		OCTREE_BINDING_OBJECT,
		OCTREE_BINDING_CAMERA
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
	IKRenderTargetPtr m_VoxelTexMipmap[6];

	IKRenderTargetPtr m_LightPassTarget;
	IKRenderPassPtr m_LightPassRenderPass;

	IKRenderTargetPtr m_OctreeRayTestTarget;
	IKRenderPassPtr m_OctreeRayTestPass;

	IKSamplerPtr m_CloestSampler;
	IKSamplerPtr m_LinearSampler;
	IKSamplerPtr m_MipmapSampler;

	IKStorageBufferPtr m_CounterBuffer;
	IKStorageBufferPtr m_FragmentlistBuffer;
	IKStorageBufferPtr m_CountOnlyBuffer;

	IKStorageBufferPtr m_OctreeBuffer;
	IKStorageBufferPtr m_OctreeDataBuffer;
	IKStorageBufferPtr m_OctreeMipmapDataBuffer;
	IKStorageBufferPtr m_BuildInfoBuffer;
	IKStorageBufferPtr m_BuildIndirectBuffer;
	IKStorageBufferPtr m_OctreeCameraBuffer;

	IKComputePipelinePtr m_OctreeTagNodePipeline;
	IKComputePipelinePtr m_OctreeInitNodePipeline;
	IKComputePipelinePtr m_OctreeAllocNodePipeline;
	IKComputePipelinePtr m_OctreeModifyArgPipeline;

	IKComputePipelinePtr m_OctreeInitDataPipeline;
	IKComputePipelinePtr m_OctreeAssignDataPipeline;

	uint32_t m_VolumeDimension;
	uint32_t m_VoxelCount;
	uint32_t m_NumMipmap;
	uint32_t m_OctreeLevel;
	uint32_t m_OctreeNonLeafCount;
	uint32_t m_OctreeLeafCount;
	glm::vec3 m_VolumeCenter;
	glm::vec3 m_VolumeMin;
	glm::vec3 m_VolumeMax;
	float m_VolumeGridSize;
	float m_VoxelSize;

	glm::mat4 m_ViewProjectionMatrix[3];
	glm::mat4 m_ViewProjectionMatrixI[3];

	IKRenderTargetPtr m_VoxelRenderPassTarget;
	IKRenderPassPtr m_VoxelRenderPass;

	KShaderRef m_VoxelDrawVS;
	KShaderRef m_VoxelDrawOctreeVS;
	KShaderRef m_VoxelDrawGS;
	KShaderRef m_VoxelWireFrameDrawGS;
	KShaderRef m_VoxelDrawFS;

	IKPipelinePtr m_VoxelDrawPipeline;
	IKPipelinePtr m_VoxelWireFrameDrawPipeline;

	IKPipelinePtr m_VoxelDrawOctreePipeline;
	IKPipelinePtr m_VoxelWireFrameDrawOctreePipeline;

	IKComputePipelinePtr m_ClearDynamicPipeline;

	IKComputePipelinePtr m_InjectRadiancePipeline;
	IKComputePipelinePtr m_InjectPropagationPipeline;

	IKComputePipelinePtr m_InjectRadianceOctreePipeline;
	IKComputePipelinePtr m_InjectPropagationOctreePipeline;

	IKComputePipelinePtr m_MipmapBasePipeline;
	IKComputePipelinePtr m_MipmapBaseOctreePipeline;
	IKComputePipelinePtr m_MipmapVolumePipeline;

	IKComputePipelinePtr m_OctreeMipmapBasePipeline;
	IKComputePipelinePtr m_OctreeMipmapVolumePipeline;

	KShaderRef m_QuadVS;
	KShaderRef m_LightPassFS;
	KShaderRef m_LightPassOctreeFS;
	KShaderRef m_OctreeRayTestFS;

	IKPipelinePtr m_LightPassPipeline;
	IKPipelinePtr m_LightPassOctreePipeline;
	IKPipelinePtr m_OctreeRayTestPipeline;

	KVertexData m_VoxelDrawVertexData;

	EntityObserverFunc m_OnSceneChangedFunc;

	bool m_Enable;

	bool m_InjectFirstBounce;
	bool m_VoxelDrawEnable;
	bool m_VoxelDrawWireFrame;
	bool m_VoxelDebugUpdate;
	bool m_VoxelNeedUpdate;
	bool m_VoxelUseOctree;
	bool m_VoxelLastUseOctree;

	KRTDebugDrawer m_LightDebugDrawer;
	KRTDebugDrawer m_OctreeRayTestDebugDrawer;

	void OnSceneChanged(EntitySceneOp op, IKEntity* entity);
	void UpdateProjectionMatrices();

	void SetupVoxelBuffer();
	void SetupSparseVoxelBuffer();
	void SetupVoxelReleatedData();

	void SetupVoxelVolumes(uint32_t dimension);
	void SetupVoxelDrawPipeline();
	void SetupClearDynamicPipeline();
	void SetupRadiancePipeline();
	void SetupMipmapPipeline();
	void SetupOctreeMipmapPipeline();
	void SetupLightPassPipeline();

	void ClearDynamicScene(IKCommandBufferPtr commandBuffer);
	void VoxelizeStaticScene(IKCommandBufferPtr commandBuffer);
	void UpdateRadiance(IKCommandBufferPtr commandBuffer);
	void InjectRadiance(IKCommandBufferPtr commandBuffer);
	void GenerateMipmap(IKCommandBufferPtr commandBuffer);
	void GenerateMipmapBase(IKCommandBufferPtr commandBuffer);
	void GenerateMipmapVolume(IKCommandBufferPtr commandBuffer);
	void GenerateOctreeMipmapBase(IKCommandBufferPtr commandBuffer);
	void GenerateOctreeMipmapVolume(IKCommandBufferPtr commandBuffer);

	void SetupOctreeBuildPipeline();
	void SetupRayTestPipeline(uint32_t width, uint32_t height);
	void VoxelizeStaticSceneCounter(IKCommandBufferPtr commandBuffer, bool countOnly);
	void CheckFragmentlistData();
	void CheckOctreeData();
	void BuildOctree(IKCommandBufferPtr commandBuffer);
	void ShrinkOctree();

	void UpdateInternal(IKCommandBufferPtr primaryBuffer);

	bool UpdateLightingResult(IKCommandBufferPtr primaryBuffer);
	bool UpdateOctreRayTestResult(IKCommandBufferPtr primaryBuffer);
public:
	KVoxilzer();
	~KVoxilzer();

	void UpdateVoxel(IKCommandBufferPtr primaryBuffer);
	void ReloadShader();
	
	bool& GetEnable() { return m_Enable; }
	bool& GetVoxelUseOctree() { return m_VoxelUseOctree; }
	bool& GetVoxelDebugUpdate() { return m_VoxelDebugUpdate; }
	bool& GetVoxelDrawEnable() { return m_VoxelDrawEnable; }
	bool& GetVoxelDrawWireFrame() { return m_VoxelDrawWireFrame; }
	bool& GetLightDebugDrawEnable() { return m_LightDebugDrawer.GetEnable(); }
	bool& GetOctreeRayTestDrawEnable() { return m_OctreeRayTestDebugDrawer.GetEnable(); }

	inline bool IsVoxelDebugUpdate() const { return m_VoxelDebugUpdate; }
	inline void SetVoxelDebugUpdate(bool enable) { m_VoxelDebugUpdate = enable; }

	inline bool IsVoxelDrawEnable() const { return m_VoxelDrawEnable; }
	inline void SetVoxelDrawEnable(bool enable) { m_VoxelDrawEnable = enable; }

	inline bool IsVoxelDrawWireFrame() const { return m_VoxelDrawWireFrame; }
	inline void SetVoxelDrawWireFrame(bool wireframe) { m_VoxelDrawWireFrame = wireframe; }

	bool EnableLightDebugDraw();
	bool DisableLightDebugDraw();
	bool DebugRender(IKRenderPassPtr renderPass, IKCommandBufferPtr primaryBuffer);

	bool EnableOctreeRayTestDebugDraw();
	bool DisableOctreeRayTestDebugDraw();
	bool OctreeRayTestRender(IKRenderPassPtr renderPass, IKCommandBufferPtr primaryBuffer);

	void Resize(uint32_t width, uint32_t height);
	bool RenderVoxel(IKRenderPassPtr renderPass, IKCommandBufferPtr primaryBuffer);
	bool UpdateFrame(IKCommandBufferPtr primaryBuffer);

	IKFrameBufferPtr GetStaticFlag() { return m_StaticFlag ? m_StaticFlag->GetFrameBuffer() : nullptr; }
	IKFrameBufferPtr GetVoxelAlbedo() { return m_VoxelAlbedo ? m_VoxelAlbedo->GetFrameBuffer() : nullptr; }
	IKFrameBufferPtr GetVoxelNormal() { return m_VoxelNormal ? m_VoxelNormal->GetFrameBuffer() : nullptr; }
	IKFrameBufferPtr GetVoxelEmissive() { return m_VoxelEmissive ? m_VoxelEmissive->GetFrameBuffer() : nullptr; }
	IKFrameBufferPtr GetVoxelRadiance() { return m_VoxelRadiance ? m_VoxelRadiance->GetFrameBuffer() : nullptr; }
	IKFrameBufferPtr GetVoxelTexMipmap(uint32_t face) { return (face < 6 && m_VoxelTexMipmap[face]) ? m_VoxelTexMipmap[face]->GetFrameBuffer() : nullptr; }

	IKStorageBufferPtr GetCounterBuffer() { return m_CounterBuffer; }
	IKStorageBufferPtr GetFragmentlistBuffer() { return m_FragmentlistBuffer; }
	IKStorageBufferPtr GetCountOnlyBuffer() { return m_CountOnlyBuffer; }

	bool Init(IKRenderScene* scene, const KCamera* camera, uint32_t dimension, uint32_t width, uint32_t height);
	bool UnInit();

	inline IKRenderTargetPtr GetFinalMask() { return m_LightPassTarget; }
};