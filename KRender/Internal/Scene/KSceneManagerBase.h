#pragma once
#include "Interface/IKRenderScene.h"
#include <deque>

class KCamera;

class KSceneManagerBase
{
public:
	virtual ~KSceneManagerBase() {}
	virtual SceneManagerType GetType() = 0;
	virtual bool Add(IKEntityPtr entity) = 0;
	virtual bool Remove(IKEntityPtr entity) = 0;
	virtual bool Move(IKEntityPtr entity) = 0;
	virtual bool GetVisibleEntity(const KCamera* camera, std::deque<IKEntityPtr>& visibles) = 0;
	virtual bool GetVisibleEntity(const KAABBBox* bound, std::deque<IKEntityPtr>& visibles) = 0;
	virtual bool GetDebugEntity(std::deque<IKEntityPtr>& debugVisibles) = 0;
	virtual bool GetSceneBound(KAABBBox& box) = 0;

	virtual bool Pick(const glm::vec3& origin, const glm::vec3& dir, std::vector<IKEntityPtr>& result) = 0;
};