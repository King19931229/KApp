#include "KRenderScene.h"
#include "KOctreeSceneManager.h"

EXPORT_DLL IKRenderScenePtr CreateRenderScene()
{
	return IKRenderScenePtr(KNEW KRenderScene());
}

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

void KRenderScene::OnEntityChange(EntitySceneOp op, IKEntityPtr entity)
{
	for (EntityObserverFunc* observer : m_Observers)
	{
		(*observer)(op, entity);
	}
}

bool KRenderScene::Add(IKEntityPtr entity)
{
	if (m_SceneMgr)
	{
		// TODO处理返回值
		m_SceneMgr->Add(entity);
		OnEntityChange(ESO_ADD, entity);
		return true;
	}
	return false;
}

bool KRenderScene::Remove(IKEntityPtr entity)
{
	if (m_SceneMgr)
	{
		// TODO处理返回值
		m_SceneMgr->Remove(entity);
		OnEntityChange(ESO_REMOVE, entity);
		return true;
	}
	return false;
}

bool KRenderScene::Move(IKEntityPtr entity)
{
	if(m_SceneMgr)
	{
		// TODO处理返回值
		m_SceneMgr->Move(entity);
		OnEntityChange(ESO_MOVE, entity);
		return true;
	}
	return false;
}

bool KRenderScene::RegisterEntityObserver(EntityObserverFunc* func)
{
	if (func)
	{
		m_Observers.insert(func);
		return true;
	}
	return false;
}

bool KRenderScene::UnRegisterEntityObserver(EntityObserverFunc* func)
{
	if (func)
	{
		m_Observers.erase(func);
		return true;
	}
	else
	{
		return false;
	}
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

		if (camera.CalcPickRay(x, y, screenWidth, screenHeight, origin, dir))
		{
			return RayPick(origin, dir, result);
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

		if (camera.CalcPickRay(x, y, screenWidth, screenHeight, origin, dir))
		{
			return CloestRayPick(origin, dir, result);
		}
	}

	return false;
}

bool KRenderScene::RayPick(const glm::vec3& origin, const glm::vec3& dir, std::vector<IKEntityPtr>& result)
{
	if (m_SceneMgr)
	{
		std::vector<IKEntityPtr> entities;
		if (m_SceneMgr->Pick(origin, dir, entities))
		{
			result.clear();
			result.reserve(entities.size());

			for (IKEntityPtr entity : entities)
			{
				glm::vec3 resultPoint;
				if (entity->Intersect(origin, dir, resultPoint))
				{
					result.push_back(entity);
				}
			}

			return !result.empty();
		}
	}
	return false;
}

bool KRenderScene::CloestRayPick(const glm::vec3& origin, const glm::vec3& dir, IKEntityPtr& result)
{
	if (m_SceneMgr)
	{
		std::vector<IKEntityPtr> entities;
		if (m_SceneMgr->Pick(origin, dir, entities))
		{
			for (IKEntityPtr entity : entities)
			{
				glm::vec3 resultPoint;
				float maxDistance = std::numeric_limits<float>::max();
				result = nullptr;

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
	return false;
}

bool KRenderScene::GetAllEntities(std::vector<IKEntityPtr>& result)
{
	result.clear();
	if (m_SceneMgr)
	{
		std::deque<IKEntityPtr> entities;
		m_SceneMgr->GetAllEntity(entities);
		result.reserve(entities.size());
		for (IKEntityPtr entity : entities)
		{
			result.push_back(entity);
		}
		return true;
	}
	return false;
}