#pragma once
#include "KVirtualGeomerty.h"
#include "KVirtualGeometryStreaming.h"
#include "KBase/Interface/Entity/IKEntity.h"
#include "KBase/Interface/Component/IKRenderComponent.h"
#include "Interface/IKRenderScene.h"
#include "Interface/IKBuffer.h"
#include <list>

class KVirtualGeometryScene : public IKVirtualGeometryScene
{
protected:
	static constexpr char* VIRTUAL_GEOMETRY_SCENE_GLOBAL_DATA = "VirtualGeometrySceneGlobalData";
	static constexpr char* VIRTUAL_GEOMETRY_SCENE_INSTANCE_DATA = "VirtualGeometrySceneInstanceData";
	static constexpr char* VIRTUAL_GEOMETRY_SCENE_MAIN_CULL_RESULT = "VirtualGeometrySceneMainCullResult";
	static constexpr char* VIRTUAL_GEOMETRY_SCENE_POST_CULL_INDIRECT_ARGS = "VirtualGeometryScenePostCullIndirectArgs";
	static constexpr char* VIRTUAL_GEOMETRY_SCENE_QUEUE_STATE = "VirtualGeometrySceneQueueState";
	static constexpr char* VIRTUAL_GEOMETRY_SCENE_CANDIDATE_NODE = "VirtualGeometrySceneCandidateNode";
	static constexpr char* VIRTUAL_GEOMETRY_SCENE_CANDIDATE_CLUSTER = "VirtualGeometrySceneCandidateCluster";
	static constexpr char* VIRTUAL_GEOMETRY_SCENE_INDIRECT_ARGS = "VirtualGeometrySceneIndirectArgs";
	static constexpr char* VIRTUAL_GEOMETRY_SCENE_SELECTED_CLUSTER = "VirtualGeometrySceneSelectedCluster";
	static constexpr char* VIRTUAL_GEOMETRY_SCENE_BINDING_EXTRA_DEBUG_INFO = "VirtualGeometrySceneDebugExtraInfo";
	static constexpr char* VIRTUAL_GEOMETRY_SCENE_INDIRECT_DRAW_ARGS = "VirtualGeometrySceneIndirectDrawArgs";
	static constexpr char* VIRTUAL_GEOMETRY_SCENE_INDIRECT_MESH_ARGS = "VirtualGeometrySceneIndirectMeshArgs";
	static constexpr char* VIRTUAL_GEOMETRY_SCENE_BINNING_DATA = "VirtualGeometrySceneBinningData";
	static constexpr char* VIRTUAL_GEOMETRY_SCENE_BINNIIG_HEADER = "VirtualGeometrySceneBinningHeader";

	IKRenderScene* m_Scene;
	const KCamera* m_Camera;

	struct Instance
	{
		uint32_t index = 0;
		glm::mat4 prevTransform;
		glm::mat4 transform;
		KVirtualGeometryResourceRef resource;
	};
	typedef std::shared_ptr<Instance> InstancePtr;

	std::unordered_map<IKEntity*, InstancePtr> m_InstanceMap;
	std::vector<InstancePtr> m_Instances;

	std::vector<KVirtualGeometryInstance> m_LastInstanceData;

	enum BinningPipelineIndex
	{
		BINNIING_PIPELINE_VERTEX,
		BINNIING_PIPELINE_MESH,
		BINNIING_PIPELINE_COUNT
	};

	enum InstanceCull
	{
		INSTANCE_CULL_NONE,
		INSTANCE_CULL_MAIN,
		INSTANCE_CULL_POST,
		INSTANCE_CULL_COUNT
	};

	KShaderRef m_BasePassVertexShader;
	KShaderRef m_BasePassMeshShader;

	std::vector<KMaterialRef> m_BinningMaterials;
	std::vector<IKPipelinePtr> m_BinningPipelines[BINNIING_PIPELINE_COUNT];
	std::vector<KShaderRef> m_BasePassFragmentShaders[BINNIING_PIPELINE_COUNT];

	IKUniformBufferPtr m_GlobalDataBuffer;

	IKStorageBufferPtr m_InstanceDataBuffer;
	IKStorageBufferPtr m_QueueStateBuffer;
	IKStorageBufferPtr m_CandidateNodeBuffer;
	IKStorageBufferPtr m_CandidateClusterBuffer;

	IKStorageBufferPtr m_IndirectAgrsBuffer;

	IKStorageBufferPtr m_SelectedClusterBuffer;
	IKStorageBufferPtr m_ExtraDebugBuffer;

	IKStorageBufferPtr m_IndirectDrawBuffer;
	IKStorageBufferPtr m_IndirectMeshBuffer;

	IKStorageBufferPtr m_BinningDataBuffer;
	IKStorageBufferPtr m_BinningHeaderBuffer;
	IKStorageBufferPtr m_MainCullResultBuffer;

	IKStorageBufferPtr m_PostCullIndirectArgsBuffer;

	IKComputePipelinePtr m_InitQueueStatePipeline[INSTANCE_CULL_COUNT];
	IKComputePipelinePtr m_InstanceCullPipeline[INSTANCE_CULL_COUNT];
	IKComputePipelinePtr m_InitNodeCullArgsPipeline[INSTANCE_CULL_COUNT];
	IKComputePipelinePtr m_InitClusterCullArgsPipeline[INSTANCE_CULL_COUNT];

	IKComputePipelinePtr m_CalcDrawArgsPipeline[INSTANCE_CULL_COUNT];
	IKComputePipelinePtr m_InitBinningPipline[INSTANCE_CULL_COUNT];
	IKComputePipelinePtr m_BinningClassifyPipline[INSTANCE_CULL_COUNT];
	IKComputePipelinePtr m_BinningAllocatePipline[INSTANCE_CULL_COUNT];
	IKComputePipelinePtr m_BinningScatterPipline[INSTANCE_CULL_COUNT];

	std::vector<IKComputePipelinePtr> m_NodeCullPipelines[INSTANCE_CULL_COUNT];
	std::vector<IKComputePipelinePtr> m_PersistentCullPipelines[INSTANCE_CULL_COUNT];
	std::vector<IKComputePipelinePtr> m_ClusterCullPipelines[INSTANCE_CULL_COUNT];

	glm::mat4 m_PrevViewProj;

	KShaderRef m_DebugVertexShader;
	KShaderRef m_DebugFragmentShader;
	IKPipelinePtr m_DebugPipeline;

	EntityObserverFunc m_OnSceneChangedFunc;
	void OnSceneChanged(EntitySceneOp op, IKEntity* entity);

	RenderComponentObserverFunc m_OnRenderComponentChangedFunc;
	void OnRenderComponentChanged(IKRenderComponent* renderComponent, bool init);

	bool UpdateInstanceData();

	InstancePtr CreateInstance(IKEntity* entity);
	InstancePtr GetInstance(IKEntity* entity);

	bool AddInstance(IKEntity* entity, const glm::mat4& transform, KVirtualGeometryResourceRef resource);
	bool TransformInstance(IKEntity* entity, const glm::mat4& transform);
	bool RemoveInstance(IKEntity* entity);

	bool Execute(IKCommandBufferPtr primaryBuffer, InstanceCull cullMode);
	bool BasePass(IKRenderPassPtr renderPass, IKCommandBufferPtr primaryBuffer, InstanceCull cullMode);

	static std::string InstanceCullString(InstanceCull cullMode);
public:
	KVirtualGeometryScene();
	~KVirtualGeometryScene();

	bool Init(IKRenderScene* scene, const KCamera* camera) override;
	bool UnInit() override;

	bool ExecuteMain(IKCommandBufferPtr primaryBuffer) override;
	bool ExecutePost(IKCommandBufferPtr primaryBuffer) override;

	bool BasePassMain(IKRenderPassPtr renderPass, IKCommandBufferPtr primaryBuffer) override;
	bool BasePassPost(IKRenderPassPtr renderPass, IKCommandBufferPtr primaryBuffer) override;

	bool DebugRender(IKRenderPassPtr renderPass, IKCommandBufferPtr primaryBuffer) override;

	bool ReloadShader();

	inline IKStorageBufferPtr GetInstanceBuffer() { return m_InstanceDataBuffer; }
};