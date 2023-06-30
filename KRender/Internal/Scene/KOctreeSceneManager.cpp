#include "KOctreeSceneManager.h"
#include "Internal/ECS/Component/KRenderComponent.h"
#include "Internal/ECS/Component/KTransformComponent.h"

KOctreeSceneManager::KOctreeSceneManager()
	: m_Root(nullptr),
	m_Looseness(1.0f),
	m_InitialSize(0.0f),
	m_MinSize(0.0f)
{
}

KOctreeSceneManager::~KOctreeSceneManager()
{
	assert(!m_Root);
}

bool KOctreeSceneManager::Init(float initialWorldSize, const glm::vec3& initialWorldPos, float minNodeSize, float loosenessVal)
{
	UnInit();

	m_InitialSize = initialWorldSize;
	m_MinSize = minNodeSize;
	m_Looseness = glm::clamp(loosenessVal, 1.0f, 2.0f);

	m_Root = KNEW KOctreeNode(m_InitialSize, m_MinSize, m_Looseness, initialWorldPos, &m_EntityToNode);

	return true;
}

bool KOctreeSceneManager::UnInit()
{
	m_EntityToNode.clear();
	SAFE_DELETE(m_Root);
	return true;
}

bool KOctreeSceneManager::Add(IKEntity* entity)
{
	KAABBBox bound;
	if (m_Root && entity->GetBound(bound))
	{
		return m_Root->Add(entity, bound);
	}
	return false;
}

bool KOctreeSceneManager::Remove(IKEntity* entity)
{
	if (m_Root)
	{
		auto it = m_EntityToNode.find(entity);
		if (it != m_EntityToNode.end())
		{
			KOctreeNode* node = it->second;
			bool success = node->Remove(entity);
			assert(success);
			return success;
		}
	}
	return false;
}

bool KOctreeSceneManager::Transform(IKEntity* entity)
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

bool KOctreeSceneManager::GetVisibleEntity(const KCamera* camera, std::vector<IKEntity*>& visibles)
{
	if (m_Root)
	{
		m_Root->GetWithinCamera(*camera, visibles);
		return true;
	}
	return false;
}

bool KOctreeSceneManager::GetVisibleEntity(const KAABBBox* bound, std::vector<IKEntity*>& visibles)
{
	if (m_Root)
	{
		m_Root->GetColliding(*bound, visibles);
		return true;
	}
	return false;
}

bool KOctreeSceneManager::GetAllEntity(std::vector<IKEntity*>& visibles)
{
	if (m_Root)
	{
		m_Root->GetAll(visibles);
		return true;
	}
	return false;
}

bool KOctreeSceneManager::GetDebugEntity(std::vector<IKEntity*>& debugVisibles)
{
	if (m_Root)
	{
		return true;
	}
	return false;
}

bool KOctreeSceneManager::GetSceneBound(KAABBBox& box)
{
	if (m_Root)
	{
		m_Root->GetObjectBound(box);
		return true;
	}
	return false;
}

bool KOctreeSceneManager::Pick(const glm::vec3& origin, const glm::vec3& dir, std::vector<IKEntity*>& result)
{
	if (m_Root)
	{
		m_Root->GetColliding(origin, dir, result);
		return true;
	}
	return false;
}