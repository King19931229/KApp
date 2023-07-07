#pragma once
#include "KVirtualGeomerty.h"
#include "KBase/Interface/Entity/IKEntity.h"
#include "KBase/Interface/Component/IKRenderComponent.h"
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

	std::unordered_map<IKEntity*, InstancePtr> m_InstanceMap;
	std::vector<InstancePtr> m_Instances;

	struct InstanceBufferData
	{
		glm::mat4 transform;
		uint32_t resourceIndex;
	};
	IKStorageBufferPtr m_InstanceDataBuffer;

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

	bool Init(IKRenderScene* scene, const KCamera* camera);
	bool UnInit();

	bool Update();

	inline IKStorageBufferPtr GetInstanceBuffer() { return m_InstanceDataBuffer; }
};