#include "KScene.h"
#include "KOctreeSceneManager.h"

KScene::KScene()
	: m_SceneMgr(nullptr),
	m_EnableDebugRender(false)
{
}

KScene::~KScene()
{
	assert(!m_SceneMgr);
}

bool KScene::Init(SceneManagerType type, float initialSize, const glm::vec3& initialPos)
{
	UnInit();

	switch (type)
	{
	case SCENE_MANGER_TYPE_OCTREE:
		m_SceneMgr = new KOctreeSceneManager();
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

bool KScene::UnInit()
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

bool KScene::Add(IKEntityPtr entity)
{
	return m_SceneMgr && m_SceneMgr->Add(entity);
}

bool KScene::Remove(IKEntityPtr entity)
{
	return m_SceneMgr && m_SceneMgr->Remove(entity);
}

bool KScene::Move(IKEntityPtr entity)
{
	return m_SceneMgr && m_SceneMgr->Move(entity);
}

bool KScene::GetRenderComponent(const KCamera& camera, std::vector<KRenderComponent*>& result)
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

bool KScene::Pick(const KCamera& camera, size_t x, size_t y,
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

bool KScene::CloestPick(const KCamera& camera, size_t x, size_t y,
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

bool KScene::Load(const char* filename)
{
	return false;
}

bool KScene::Save(const char* filename)
{
	return false;
}