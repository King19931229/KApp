#pragma once
#include "Interface/IKRenderScene.h"
#include <deque>

class KCamera;

class KSceneManagerBase
{
public:
	virtual ~KSceneManagerBase() {}
	virtual SceneManagerType GetType() = 0;
	virtual bool Add(IKEntity* entity) = 0;
	virtual bool Remove(IKEntity* entity) = 0;
	virtual bool Transform(IKEntity* entity) = 0;
	virtual bool GetVisibleEntity(const KCamera* camera, std::vector<IKEntity*>& visibles) = 0;
	virtual bool GetVisibleEntity(const KAABBBox* bound, std::vector<IKEntity*>& visibles) = 0;
	virtual bool GetAllEntity(std::vector<IKEntity*>& visibles) = 0;
	virtual bool GetSceneBound(KAABBBox& box) = 0;

	virtual bool Pick(const glm::vec3& origin, const glm::vec3& dir, std::vector<IKEntity*>& result) = 0;
};