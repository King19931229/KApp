#pragma once
#include "Interface/IKRenderScene.h"
#include "Interface/IKRenderConfig.h"
#include "Interface/IKTexture.h"
#include "Interface/IKRenderTarget.h"
#include "Interface/IKComputePipeline.h"
#include "Internal/KVertexDefinition.h"
#include "Internal/Object/KDebugDrawer.h"

class KVoxilzer
{
protected:
	enum
	{
		GROUP_SIZE = 8,
		OCTREE_LEVEL_MIN = 1,
		OCTREE_LEVEL_MAX = 12,
		OCTREE_NODE_NUM_MIN = 1000000,
		OCTREE_NODE_NUM_MAX = 500000000,
		BEAM_SIZE = 8
	};

	enum OctreeBuildBinding
	{
		OCTREE_BINDING_COUNTER,
		OCTREE_BINDING_OCTTREE,
		OCTREE_BINDING_FRAGMENTLIST,
		OCTREE_BINDING_BUILDINFO,
		OCTREE_BINDING_INDIRECT,
		OCTREE_BINDING_OBJECT,
		OCTREE_BINDING_CAMERA
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
	const KCamera* m_Camera;

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
	IKStorageBufferPtr m_BuildInfoBuffer;
	IKStorageBufferPtr m_BuildIndirectBuffer;
	std::vector<IKStorageBufferPtr> m_OctreeCameraBuffers;

	IKComputePipelinePtr m_OctreeTagNodePipeline;
	IKComputePipelinePtr m_OctreeInitNodePipeline;
	IKComputePipelinePtr m_OctreeAllocNodePipeline;
	IKComputePipelinePtr m_OctreeModifyArgPipeline;

	uint32_t m_VolumeDimension;
	uint32_t m_VoxelCount;
	uint32_t m_NumMipmap;
	uint32_t m_OctreeLevel;
	glm::vec3 m_VolumeCenter;
	glm::vec3 m_VolumeMin;
	glm::vec3 m_VolumeMax;
	float m_VolumeGridSize;
	float m_VoxelSize;

	glm::mat4 m_ViewProjectionMatrix[3];
	glm::mat4 m_ViewProjectionMatrixI[3];

	IKCommandBufferPtr m_PrimaryCommandBuffer;
	std::vector<IKCommandBufferPtr> m_DrawCommandBuffers;
	std::vector<IKCommandBufferPtr> m_LightingCommandBuffers;
	std::vector<IKCommandBufferPtr> m_OctreeRayTestCommandBuffers;
	IKCommandPoolPtr m_CommandPool;

	IKRenderTargetPtr m_VoxelRenderPassTarget;
	IKRenderPassPtr m_VoxelRenderPass;

	IKShaderPtr m_VoxelDrawVS;
	IKShaderPtr m_VoxelDrawGS;
	IKShaderPtr m_VoxelWireFrameDrawGS;
	IKShaderPtr m_VoxelDrawFS;

	std::vector<IKPipelinePtr> m_VoxelDrawPipelines;
	std::vector<IKPipelinePtr> m_VoxelWireFrameDrawPipelines;

	IKComputePipelinePtr m_ClearDynamicPipeline;

	IKComputePipelinePtr m_InjectRadiancePipeline;
	IKComputePipelinePtr m_InjectPropagationPipeline;

	IKComputePipelinePtr m_MipmapBasePipeline;
	IKComputePipelinePtr m_MipmapVolumePipeline;

	IKShaderPtr m_QuadVS;
	IKShaderPtr m_LightPassFS;
	IKShaderPtr m_OctreeRayTestFS;

	std::vector<IKPipelinePtr> m_LightPassPipelines;
	std::vector<IKPipelinePtr> m_OctreeRayTestPipelines;

	KVertexData m_VoxelDrawVertexData;

	EntityObserverFunc m_OnSceneChangedFunc;

	bool m_InjectFirstBounce;
	bool m_VoxelDrawEnable;
	bool m_VoxelDrawWireFrame;
	bool m_VoxelDebugUpdate;
	bool m_VoxelNeedUpdate;

	KRTDebugDrawer m_LightDebugDrawer;
	KRTDebugDrawer m_OctreeRayTestDebugDrawer;

	void OnSceneChanged(EntitySceneOp op, IKEntityPtr entity);
	void UpdateProjectionMatrices();
	void SetupVoxelVolumes(uint32_t dimension);
	void SetupVoxelDrawPipeline();
	void SetupClearDynamicPipeline();
	void SetupRadiancePipeline();
	void SetupMipmapPipeline();
	void SetupQuadDrawData();
	void SetupLightPassPipeline(uint32_t width, uint32_t height);

	void ClearDynamicScene(IKCommandBufferPtr commandBuffer);
	void VoxelizeStaticScene(IKCommandBufferPtr commandBuffer);
	void UpdateRadiance(IKCommandBufferPtr commandBuffer);
	void InjectRadiance(IKCommandBufferPtr commandBuffer);
	void GenerateMipmap(IKCommandBufferPtr commandBuffer);
	void GenerateMipmapBase(IKCommandBufferPtr commandBuffer);
	void GenerateMipmapVolume(IKCommandBufferPtr commandBuffer);

	void SetupSparseVoxelBuffer();
	void SetupOctreeBuildPipeline();
	void SetupRayTestPipeline(uint32_t width, uint32_t height);
	void VoxelizeStaticSceneCounter(IKCommandBufferPtr commandBuffer, bool countOnly);
	void BuildOctree(IKCommandBufferPtr commandBuffer);

	void UpdateInternal();

	bool UpdateLightingResult(IKCommandBufferPtr primaryBuffer, uint32_t frameIndex);
	bool UpdateOctreRayTestResult(IKCommandBufferPtr primaryBuffer, uint32_t frameIndex);
public:
	KVoxilzer();
	~KVoxilzer();

	void UpdateVoxel();
	void ReloadShader();

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
	bool GetLightDebugRenderCommand(KRenderCommandList& commands);

	bool EnableOctreeRayTestDebugDraw();
	bool DisableOctreeRayTestDebugDraw();
	bool GetOctreeRayTestRenderCommand(KRenderCommandList& commands);

	void Resize(uint32_t width, uint32_t height);
	bool RenderVoxel(size_t frameIndex, IKRenderPassPtr renderPass, std::vector<IKCommandBufferPtr>& buffers);
	bool UpdateFrame(IKCommandBufferPtr primaryBuffer, uint32_t frameIndex);

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
};