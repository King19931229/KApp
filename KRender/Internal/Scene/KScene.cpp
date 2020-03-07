#include "KScene.h"
#include "KOctreeSceneManager.h"

KScene::KScene()
	: m_SceneMgr(nullptr)
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

bool KScene::Add(KEntityPtr entity)
{
	return m_SceneMgr && m_SceneMgr->Add(entity);
}

bool KScene::Remove(KEntityPtr entity)
{
	return m_SceneMgr && m_SceneMgr->Remove(entity);
}

bool KScene::Move(KEntityPtr entity)
{
	return m_SceneMgr && m_SceneMgr->Move(entity);
}

bool KScene::GetVisibleComponent(const KCamera& camera, std::vector<KRenderComponent*>& result)
{
	std::deque<KEntityPtr> visibles;
	if (m_SceneMgr && m_SceneMgr->GetVisibleEntity(&camera, visibles))
	{
		result.clear();
		result.reserve(visibles.size());

		for (KEntityPtr entity : visibles)
		{
			KRenderComponent* component = nullptr;
			if (entity->GetComponent(CT_RENDER, (KComponentBase**)&component) && component)
			{
				result.push_back(component);
			}
		}

		return true;
	}
	return false;
}

bool KScene::GetDebugComponent(std::vector<KRenderComponent*>& result)
{
	result.clear();
	if (m_SceneMgr && m_SceneMgr->GetDebugComponent(result))
	{
		return true;
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