#pragma once
#include "KBase/Publish/KConfig.h"
#include "KOctreeNode.h"
#include "KSceneManagerBase.h"

class KOctreeSceneManager : public KSceneManagerBase
{
protected:
	// Root node of the octree
	KOctreeNode* m_Root;
	// Should be a value between 1 and 2. A multiplier for the base size of a node.
	// 1.0 is a "normal" octree, while values > 1 have overlap
	float m_Looseness;
	// Size that the octree was on creation
	float m_InitialSize;
	// Minimum side length that a node can be - essentially an alternative to having a max depth
	float m_MinSize;

	KEntityToNodeMap m_EntityToNode;
public:
	KOctreeSceneManager();
	~KOctreeSceneManager();

	bool Init(float initialWorldSize, const glm::vec3& initialWorldPos, float minNodeSize, float loosenessVal);
	bool UnInit();

	SceneManagerType GetType() override { return SCENE_MANGER_TYPE_OCTREE; }
	bool Add(IKEntity* entity) override;
	bool Remove(IKEntity* entity) override;
	bool Transform(IKEntity* entity) override;
	bool GetVisibleEntity(const KCamera* camera, std::vector<IKEntity*>& visibles) override;
	bool GetVisibleEntity(const KAABBBox* bound, std::vector<IKEntity*>& visibles) override;
	bool GetAllEntity(std::vector<IKEntity*>& visibles) override;
	bool GetSceneBound(KAABBBox& box) override;

	bool Pick(const glm::vec3& origin, const glm::vec3& dir, std::vector<IKEntity*>& result) override;
};