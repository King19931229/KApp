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
	virtual bool Add(KEntityPtr entity) = 0;
	virtual bool Remove(KEntityPtr entity) = 0;
	virtual bool Move(KEntityPtr entity) = 0;
	virtual bool GetVisibleEntity(const KCamera* camera, std::deque<KEntityPtr>& visibles) = 0;
	virtual bool GetDebugComponent(std::vector<KRenderComponent*>& result) = 0;
};