#include "KRenderScene.h"
#include "KOctreeSceneManager.h"

KRenderScene::KRenderScene()
	: m_SceneMgr(nullptr),
	m_EnableDebugRender(false)
{
}

KRenderScene::~KRenderScene()
{
	assert(!m_SceneMgr);
}

bool KRenderScene::Init(SceneManagerType type, float initialSize, const glm::vec3& initialPos)
{
	UnInit();

	switch (type)
	{
	case SCENE_MANGER_TYPE_OCTREE:
		m_SceneMgr = KNEW KOctreeSceneManager();
		break;
	default:
		assert(false && "impossible");
		break;
	}

	if (m_SceneMgr)
	{
		if (m_SceneMgr->GetType() == SCENE_MANGER_TYPE_OCTREE)
		{
			KOctreeSceneManager* octMgr = (KOctreeSceneManager*)m_SceneMgr;
			octMgr->Init(initialSize, initialPos, 1, 1.25f);
		}
		return true;
	}
	return false;
}

bool KRenderScene::UnInit()
{
	if (m_SceneMgr)
	{
		if (m_SceneMgr->GetType() == SCENE_MANGER_TYPE_OCTREE)
		{
			KOctreeSceneManager* octMgr = (KOctreeSceneManager*)m_SceneMgr;
			octMgr->UnInit();
		}
		SAFE_DELETE(m_SceneMgr);
	}
	return true;
}

bool KRenderScene::Add(IKEntityPtr entity)
{
	return m_SceneMgr && m_SceneMgr->Add(entity);
}

bool KRenderScene::Remove(IKEntityPtr entity)
{
	return m_SceneMgr && m_SceneMgr->Remove(entity);
}

bool KRenderScene::Move(IKEntityPtr entity)
{
	return m_SceneMgr && m_SceneMgr->Move(entity);
}

bool KRenderScene::GetRenderComponent(const KCamera& camera, std::vector<KRenderComponent*>& result)
{
	std::deque<IKEntityPtr> entities;
	if (m_SceneMgr)
	{
		m_SceneMgr->GetVisibleEntity(&camera, entities);

		if (m_EnableDebugRender)
		{
			m_SceneMgr->GetDebugEntity(entities);
		}

		result.clear();
		result.reserve(entities.size());

		for (IKEntityPtr entity : entities)
		{
			KRenderComponent* component = nullptr;
			if (entity->GetComponent(CT_RENDER, (IKComponentBase**)&component) && component)
			{
				result.push_back(component);
			}
		}

		return true;
	}
	return false;
}

bool KRenderScene::GetRenderComponent(const KAABBBox& bound, std::vector<KRenderComponent*>& result)
{
	std::deque<IKEntityPtr> entities;
	if (m_SceneMgr)
	{
		m_SceneMgr->GetVisibleEntity(&bound, entities);

		if (m_EnableDebugRender)
		{
			m_SceneMgr->GetDebugEntity(entities);
		}

		result.clear();
		result.reserve(entities.size());

		for (IKEntityPtr entity : entities)
		{
			KRenderComponent* component = nullptr;
			if (entity->GetComponent(CT_RENDER, (IKComponentBase**)&component) && component)
			{
				result.push_back(component);
			}
		}

		return true;
	}
	return false;
}

bool KRenderScene::GetSceneObjectBound(KAABBBox& box)
{
	if (m_SceneMgr)
	{
		return m_SceneMgr->GetSceneBound(box);
	}
	return false;
}

bool KRenderScene::Pick(const KCamera& camera, size_t x, size_t y,
	size_t screenWidth, size_t screenHeight, std::vector<IKEntityPtr>& result)
{
	if (m_SceneMgr)
	{
		glm::vec3 origin;
		glm::vec3 dir;
		glm::vec3 resultPoint;

		if (camera.CalcPickRay(x, y, screenWidth, screenHeight, origin, dir))
		{
			std::vector<IKEntityPtr> entities;
			if (m_SceneMgr->Pick(origin, dir, entities))
			{
				result.clear();
				result.reserve(entities.size());

				for (IKEntityPtr entity : entities)
				{
					if (entity->Intersect(origin, dir, resultPoint))
					{
						result.push_back(entity);
					}
				}

				return !result.empty();
			}
		}
	}

	return false;
}

bool KRenderScene::CloestPick(const KCamera& camera, size_t x, size_t y,
	size_t screenWidth, size_t screenHeight, IKEntityPtr& result)
{
	if (m_SceneMgr)
	{
		glm::vec3 origin;
		glm::vec3 dir;
		glm::vec3 resultPoint;

		float maxDistance = std::numeric_limits<float>::max();
		result = nullptr;

		if (camera.CalcPickRay(x, y, screenWidth, screenHeight, origin, dir))
		{
			std::vector<IKEntityPtr> entities;
			if (m_SceneMgr->Pick(origin, dir, entities))
			{
				for (IKEntityPtr entity : entities)
				{
					if (entity->Intersect(origin, dir, resultPoint, &maxDistance))
					{
						maxDistance = glm::dot(resultPoint - origin, dir);
						assert(maxDistance >= 0.0f);
						result = entity;
					}
				}

				return result != nullptr;
			}
		}
	}

	return false;
}