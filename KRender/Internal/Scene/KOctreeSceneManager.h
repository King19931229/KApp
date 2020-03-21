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

	KEntityToNodeMap* m_EntityToNode;
public:
	KOctreeSceneManager();
	~KOctreeSceneManager();

	bool Init(float initialWorldSize, const glm::vec3& initialWorldPos, float minNodeSize, float loosenessVal);
	bool UnInit();

	SceneManagerType GetType() override { return SCENE_MANGER_TYPE_OCTREE; }
	bool Add(KEntityPtr entity) override;
	bool Remove(KEntityPtr entity) override;
	bool Move(KEntityPtr entity) override;
	bool GetVisibleEntity(const KCamera* camera, std::deque<KEntityPtr>& visibles) override;
	bool GetDebugEntity(std::deque<KEntityPtr>& debugVisibles) override;
};