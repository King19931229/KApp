#pragma once
#include "KBase/Publish/KConfig.h"
#include "Internal/ECS/KEntity.h"
#include "Publish/KCamera.h"
#include "glm/glm.hpp"

struct KOctreeNode
{
	KOctreeNode* children[8];
	glm::vec3 center;
	glm::vec3 extend;
	KAABBBox box;

	KOctreeNode()
	{
		ZERO_ARRAY_MEMORY(children);
		center = glm::vec3(0.0f, 0.0f, 0.0f);
		extend = glm::vec3(0.0f, 0.0f, 0.0f);
		box.InitFromExtent(center, extend);
	}
};

class KOctreeSceneManager
{
	KOctreeNode* m_Root;
public:
	bool Init(const glm::vec3& center, const glm::vec3& extend, float leafSize);
	bool UnInit();

	bool Add(KEntity* entity);
	bool Remove(KEntity* entity);
	bool Update();
	bool GetVisibleEntity(const KCamera* camera, std::vector<KEntity*>& visibles);
};