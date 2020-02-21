#pragma once
#include <deque>
class KEntity;
class KCamera;

enum SceneManagerType
{
	SCENE_MANGER_TYPE_OCTREE
};

class KSceneManagerBase
{
public:
	virtual ~KSceneManagerBase() {}
	virtual SceneManagerType GetType() = 0;
	virtual bool Add(KEntity* entity) = 0;
	virtual bool Remove(KEntity* entity) = 0;
	virtual bool Move(KEntity* entity) = 0;
	virtual bool GetVisibleEntity(const KCamera* camera, std::deque<KEntity*>& visibles) = 0;
	virtual bool GetDebugComponent(std::vector<KRenderComponent*>& result) = 0;
};