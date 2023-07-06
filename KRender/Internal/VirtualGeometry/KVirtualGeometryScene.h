#pragma once
#include "KVirtualGeomerty.h"
#include "Interface/IKRenderScene.h"
#include "Interface/IKBuffer.h"
#include <list>

typedef uint32_t KVirtualGeometrySceneID;
constexpr uint32_t INVALID_VIRTUAL_GEOMETRY_SCENE_ID = std::numeric_limits<uint32_t>::max();

class KVirtualGeometryScene
{
protected:
	IKRenderScene* m_Scene;
	const KCamera* m_Camera;

	struct Instance
	{
		uint32_t index = 0;
		glm::mat4 transform;
		KVirtualGeometryResourceRef resource;
	};
	typedef std::shared_ptr<Instance> InstancePtr;

	std::string m_Name;

	std::unordered_map<KVirtualGeometrySceneID, InstancePtr> m_InstanceMap;
	std::vector<InstancePtr> m_Instances;

	std::list<KVirtualGeometrySceneID> m_UnusedIDS;
	KVirtualGeometrySceneID m_IDCounter;

	struct InstanceBufferData
	{
		glm::mat4 transform;
		uint32_t resourceIndex;
	};
	IKStorageBufferPtr m_InstanceDataBuffer;

	EntityObserverFunc m_OnSceneChangedFunc;
	void OnSceneChanged(EntitySceneOp op, IKEntity* entity);

	bool UpdateInstanceData();

	void RecycleID(KVirtualGeometrySceneID ID);
	KVirtualGeometrySceneID ObtainID();
public:
	KVirtualGeometryScene();
	~KVirtualGeometryScene();

	bool Init(IKRenderScene* scene);
	bool UnInit();

	bool Update();

	bool AddInstance(const glm::mat4& transform, KVirtualGeometryResourceRef resource, KVirtualGeometrySceneID& ID);
	bool RemoveInstance(KVirtualGeometrySceneID ID);

	inline IKStorageBufferPtr GetInstanceBuffer() { return m_InstanceDataBuffer; }
};