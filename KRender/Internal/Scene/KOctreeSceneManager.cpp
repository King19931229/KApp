#include "KOctreeSceneManager.h"
#include "Internal/ECS/Component/KRenderComponent.h"
#include "Internal/ECS/Component/KTransformComponent.h"

KOctreeSceneManager::KOctreeSceneManager()
	: m_Root(nullptr),
	m_Looseness(1.0f),
	m_InitialSize(0.0f),
	m_MinSize(0.0f),
	m_EntityToNode(nullptr)
{
}

KOctreeSceneManager::~KOctreeSceneManager()
{
	assert(!m_Root);
	assert(!m_EntityToNode);
}

bool KOctreeSceneManager::Init(float initialWorldSize, const glm::vec3& initialWorldPos, float minNodeSize, float loosenessVal)
{
	UnInit();

	m_InitialSize = initialWorldSize;
	m_MinSize = minNodeSize;
	m_Looseness = glm::clamp(loosenessVal, 1.0f, 2.0f);
	m_EntityToNode = new KEntityToNodeMap();

	m_Root = new KOctreeNode(m_InitialSize, m_MinSize, m_Looseness, initialWorldPos, m_EntityToNode);

	return true;
}

bool KOctreeSceneManager::UnInit()
{
	SAFE_DELETE(m_Root);
	SAFE_DELETE(m_EntityToNode);
	return true;
}

bool KOctreeSceneManager::Add(IKEntityPtr entity)
{
	KAABBBox bound;
	if (m_Root && entity->GetBound(bound))
	{
		return m_Root->Add(entity, bound);
	}
	return false;
}

bool KOctreeSceneManager::Remove(IKEntityPtr entity)
{
	if (m_Root && m_EntityToNode)
	{
		auto it = m_EntityToNode->find(entity);
		if (it != m_EntityToNode->end())
		{
			KOctreeNode* node = it->second;
			bool success = node->Remove(entity);
			assert(success);
			return success;
		}
	}
	return false;
}

bool KOctreeSceneManager::Move(IKEntityPtr entity)
{
	if (!Remove(entity))
	{
		return false;
	}
	if (!Add(entity))
	{
		return false;
	}
	return true;
}

bool KOctreeSceneManager::GetVisibleEntity(const KCamera* camera, std::deque<IKEntityPtr>& visibles)
{
	if (m_Root)
	{
		m_Root->GetWithinCamera(*camera, visibles);
		return true;
	}
	return false;
}

bool KOctreeSceneManager::GetDebugEntity(std::deque<IKEntityPtr>& debugVisibles)
{
	if (m_Root)
	{
		m_Root->GetDebugRender(debugVisibles);
		return true;
	}
	return false;
}

bool KOctreeSceneManager::Pick(const glm::vec3& origin, const glm::vec3& dir, std::vector<IKEntityPtr>& result)
{
	if (m_Root)
	{
		m_Root->GetColliding(origin, dir, result);
		return true;
	}
	return false;
}