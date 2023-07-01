#pragma once
#include "KVirtualGeomerty.h"
#include "Interface/IKBuffer.h"

typedef uint32_t KVirtualGeometrySceneID;

class KVirtualGeometryScene
{
protected:
	struct Instance
	{
		uint32_t index = 0;
		KVirtualGeometryResourceRef resource;
	};

	typedef std::shared_ptr<Instance> InstancePtr;

	std::unordered_map<KVirtualGeometrySceneID, InstancePtr> m_InstanceMap;
	std::vector<InstancePtr> m_Instances;

	struct InstanceBufferData
	{
		glm::mat4 transform;
		uint32_t hierarchyOffset;
		uint32_t clusterOffset;
	};
	IKStorageBufferPtr m_InstanceDataBuffer;

	bool UpdateInstanceData();
public:
	KVirtualGeometryScene();
	~KVirtualGeometryScene();

	bool Init();
	bool UnInit();

	bool Update();

	bool AddInstance(const glm::mat4& transform, KVirtualGeometryResourceRef resource, KVirtualGeometrySceneID& ID);
	bool RemoveInstance(KVirtualGeometrySceneID ID);
};