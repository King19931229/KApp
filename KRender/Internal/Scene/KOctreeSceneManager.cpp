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

	m_Root = new KOctreeNode(m_InitialSize, m_MinSize, m_Looseness, initialWorldPos);
	return true;
}

bool KOctreeSceneManager::UnInit()
{
	SAFE_DELETE(m_Root);
	return true;
}

bool KOctreeSceneManager::GetEntityBound(KEntityPtr entity, KAABBBox& bound)
{
	if (entity)
	{
		KComponentBase* component = nullptr;
		KRenderComponent* renderComponent = nullptr;
		KTransformComponent* transformComponent = nullptr;

		if (entity->GetComponent(CT_RENDER, &component))
		{
			renderComponent = (KRenderComponent*)component;
		}
		if (entity->GetComponent(CT_TRANSFORM, &component))
		{
			transformComponent = (KTransformComponent*)component;
		}

		if (renderComponent)
		{
			KMeshPtr mesh = renderComponent->GetMesh();
			if (mesh)
			{
				const KAABBBox& localBound = mesh->GetLocalBound();
				if (transformComponent)
				{
					const auto& finalTransform = transformComponent->FinalTransform();
					localBound.Transform(finalTransform.MODEL, bound);
				}
				return true;
			}
		}
	}
	return false;
}

bool KOctreeSceneManager::Add(KEntityPtr entity)
{
	KAABBBox bound;
	if (m_Root && GetEntityBound(entity, bound))
	{
		return m_Root->Add(entity, bound);
	}
	return false;
}

bool KOctreeSceneManager::Remove(KEntityPtr entity)
{
	KAABBBox bound;
	if (m_Root && GetEntityBound(entity, bound))
	{
		return m_Root->Remove(entity, bound);
	}
	return false;
}

bool KOctreeSceneManager::Move(KEntityPtr entity)
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

bool KOctreeSceneManager::GetVisibleEntity(const KCamera* camera, std::deque<KEntityPtr>& visibles)
{
	if (m_Root)
	{
		m_Root->GetWithinCamera(*camera, visibles);
		return true;
	}
	return false;
}

bool KOctreeSceneManager::GetDebugEntity(std::deque<KEntityPtr>& debugVisibles)
{
	if (m_Root)
	{
		m_Root->GetDebugRender(debugVisibles);
		return true;
	}
	return false;
}