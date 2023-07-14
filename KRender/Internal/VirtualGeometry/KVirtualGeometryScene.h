#pragma once
#include "KVirtualGeomerty.h"
#include "KBase/Interface/Entity/IKEntity.h"
#include "KBase/Interface/Component/IKRenderComponent.h"
#include "Interface/IKRenderScene.h"
#include "Interface/IKBuffer.h"
#include <list>

class KVirtualGeometryScene : public IKVirtualGeometryScene
{
protected:
	enum
	{
		MAX_CANDIDATE_NODE = 1024 * 1024,
		MAX_CANDIDATE_CLUSTERS = 1024 * 1024 * 4,

		VG_GROUP_SIZE = 64,

		BINDING_GLOBAL_DATA = 0,
		BINDING_RESOURCE,
		BINDING_QUEUE_STATE,
		BINDING_INSTANCE_DATA,
		BINDING_HIERARCHY,
		BINDING_CLUSTER_BATCH,
		BINDING_CLUSTER_STORAGE_VERTEX,
		BINDING_CLUSTER_STORAGE_INDEX,
		BINDING_CANDIDATE_NODE_BATCH,
		BINDING_CANDIDATE_CLUSTER_BATCH,
		BINDING_INDIRECT_ARGS
	};

	static constexpr char* VIRTUAL_GEOMETRY_SCENE_GLOBAL_DATA = "VirtualGeometrySceneGlobalData";
	static constexpr char* VIRTUAL_GEOMETRY_SCENE_INSTANCE_DATA = "VirtualGeometrySceneInstanceData";
	static constexpr char* VIRTUAL_GEOMETRY_SCENE_QUEUE_STATE = "VirtualGeometrySceneQueueState";
	static constexpr char* VIRTUAL_GEOMETRY_SCENE_CANDIDATE_NODE = "VirtualGeometrySceneCandidateNode";
	static constexpr char* VIRTUAL_GEOMETRY_SCENE_CANDIDATE_CLUSTER = "VirtualGeometrySceneCandidateCluster";
	static constexpr char* VIRTUAL_GEOMETRY_SCENE_INDIRECT_ARGS = "VirtualGeometrySceneIndirectArgs";

	IKRenderScene* m_Scene;
	const KCamera* m_Camera;

	struct Instance
	{
		uint32_t index = 0;
		glm::mat4 transform;
		KVirtualGeometryResourceRef resource;
	};
	typedef std::shared_ptr<Instance> InstancePtr;

	std::unordered_map<IKEntity*, InstancePtr> m_InstanceMap;
	std::vector<InstancePtr> m_Instances;

	std::vector<KVirtualGeometryInstance> m_LastInstanceData;

	IKUniformBufferPtr m_GlobalDataBuffer;

	IKStorageBufferPtr m_InstanceDataBuffer;
	IKStorageBufferPtr m_QueueStateBuffer;
	IKStorageBufferPtr m_CandidateNodeBuffer;
	IKStorageBufferPtr m_CandidateClusterBuffer;
	IKStorageBufferPtr m_IndirectAgrsBuffer;

	IKComputePipelinePtr m_InitQueueStatePipeline;
	IKComputePipelinePtr m_InstanceCullPipeline;
	IKComputePipelinePtr m_InitNodeCullArgsPipeline;
	IKComputePipelinePtr m_InitClusterCullArgsPipeline;
	IKComputePipelinePtr m_NodeCullPipeline;

	EntityObserverFunc m_OnSceneChangedFunc;
	void OnSceneChanged(EntitySceneOp op, IKEntity* entity);

	RenderComponentObserverFunc m_OnRenderComponentChangedFunc;
	void OnRenderComponentChanged(IKRenderComponent* renderComponent, bool init);

	bool UpdateInstanceData();

	InstancePtr GetOrCreateInstance(IKEntity* entity);

	bool AddInstance(IKEntity* entity, const glm::mat4& transform, KVirtualGeometryResourceRef resource);
	bool TransformInstance(IKEntity* entity, const glm::mat4& transform);
	bool RemoveInstance(IKEntity* entity);
public:
	KVirtualGeometryScene();
	~KVirtualGeometryScene();

	bool Init(IKRenderScene* scene, const KCamera* camera) override;
	bool UnInit() override;

	bool Execute(IKCommandBufferPtr primaryBuffer) override;

	bool ReloadShader();

	inline IKStorageBufferPtr GetInstanceBuffer() { return m_InstanceDataBuffer; }
};