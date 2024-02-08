#include "KRenderScene.h"
#include "KOctreeSceneManager.h"
#include "Internal/Terrain/KTerrain.h"
#include "Internal/KRenderGlobal.h"

EXPORT_DLL IKRenderScenePtr CreateRenderScene()
{
	return IKRenderScenePtr(KNEW KRenderScene());
}

KRenderScene::KRenderScene()
	: m_SceneMgr(nullptr)
{
}

KRenderScene::~KRenderScene()
{
	assert(!m_SceneMgr);
}

bool KRenderScene::Init(const std::string& name, SceneManagerType type, float initialSize, const glm::vec3& initialPos)
{
	UnInit();

	m_Name = name;
	m_Terrain = IKTerrainPtr(KNEW KNullTerrain());

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

	DestroyTerrain();
	m_Terrain = nullptr;

	return true;
}

bool KRenderScene::InitRenderResource(const KCamera* camera)
{
	KRenderGlobal::RayTraceManager.CreateRayTraceScene(m_RayTraceScene);
	m_RayTraceScene->Init(&KRenderGlobal::Scene, camera);

	KRenderGlobal::VirtualGeometryManager.CreateVirtualGeometryScene(m_VGScene);
	m_VGScene->Init(&KRenderGlobal::Scene, camera);

	KRenderGlobal::GPUScene.Init(this, camera);

	return true;
}

bool KRenderScene::UnInitRenderResource()
{
	KRenderGlobal::RayTraceManager.RemoveRayTraceScene(m_RayTraceScene);
	SAFE_UNINIT(m_RayTraceScene);

	KRenderGlobal::VirtualGeometryManager.RemoveVirtualGeometryScene(m_VGScene);
	SAFE_UNINIT(m_VGScene);

	KRenderGlobal::GPUScene.UnInit();

	return true;
}

const std::string& KRenderScene::GetName() const
{
	return m_Name;
}

void KRenderScene::OnEntityChange(EntitySceneOp op, IKEntity* entity)
{
	for (EntityObserverFunc* observer : m_Observers)
	{
		(*observer)(op, entity);
	}
}

bool KRenderScene::Add(IKEntity* entity)
{
	if (m_SceneMgr)
	{
		if (m_SceneMgr->Add(entity))
		{
			OnEntityChange(ESO_ADD, entity);
			return true;
		}
	}
	return false;
}

bool KRenderScene::Remove(IKEntity* entity)
{
	if (m_SceneMgr)
	{
		if (m_SceneMgr->Remove(entity))
		{
			OnEntityChange(ESO_REMOVE, entity);
			return true;
		}
	}
	return false;
}

bool KRenderScene::Transform(IKEntity* entity)
{
	if(m_SceneMgr)
	{
		if (m_SceneMgr->Transform(entity))
		{
			OnEntityChange(ESO_TRANSFORM, entity);
			return true;
		}
	}
	return false;
}

bool KRenderScene::CreateTerrain(const glm::vec3& center, float size, float height, const KTerrainContext& context)
{
	DestroyTerrain();
	if (context.type == TERRAIN_TYPE_CLIPMAP)
	{
		m_Terrain = IKTerrainPtr(KNEW KClipmap());
		KClipmap* clipmap = static_cast<KClipmap*>(m_Terrain.get());
		clipmap->Init(center, size, height, context.clipmap.gridLevel, context.clipmap.divideLevel);
	}
	return true;
}

bool KRenderScene::DestroyTerrain()
{
	if (m_Terrain)
	{
		if (m_Terrain->GetType() == TERRAIN_TYPE_CLIPMAP)
		{
			KClipmap* clipmap = static_cast<KClipmap*>(m_Terrain.get());
			clipmap->UnInit();
		}
		m_Terrain = IKTerrainPtr(KNEW KNullTerrain());
	}
	return true;
}

IKTerrainPtr KRenderScene::GetTerrain()
{
	return m_Terrain;
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

bool KRenderScene::GetVisibleEntities(const KCamera& camera, std::vector<IKEntity*>& result)
{
	result.clear();
	if (m_SceneMgr)
	{
		m_SceneMgr->GetVisibleEntity(&camera, result);
		return true;
	}	
	return false;
}

bool KRenderScene::GetVisibleEntities(const KAABBBox& bound, std::vector<IKEntity*>& result)
{
	result.clear();
	if (m_SceneMgr)
	{
		m_SceneMgr->GetVisibleEntity(&bound, result);
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
	size_t screenWidth, size_t screenHeight, std::vector<IKEntity*>& result)
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
	result.clear();
	return false;
}

bool KRenderScene::CloestPick(const KCamera& camera, size_t x, size_t y,
	size_t screenWidth, size_t screenHeight, IKEntity*& result)
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
	result = nullptr;
	return false;
}

bool KRenderScene::RayPick(const glm::vec3& origin, const glm::vec3& dir, std::vector<IKEntity*>& result)
{
	if (m_SceneMgr)
	{
		std::vector<IKEntity*> entities;
		if (m_SceneMgr->Pick(origin, dir, entities))
		{
			result.clear();
			result.reserve(entities.size());

			for (IKEntity* entity : entities)
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
	result.clear();
	return false;
}

bool KRenderScene::CloestRayPick(const glm::vec3& origin, const glm::vec3& dir, IKEntity*& result)
{
	if (m_SceneMgr)
	{
		std::vector<IKEntity*> entities;
		if (m_SceneMgr->Pick(origin, dir, entities))
		{
			for (IKEntity* entity : entities)
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
	result = nullptr;
	return false;
}

bool KRenderScene::GetAllEntities(std::vector<IKEntity*>& result)
{
	result.clear();
	if (m_SceneMgr)
	{
		m_SceneMgr->GetAllEntity(result);
		return true;
	}
	return false;
}

IKRayTraceScenePtr KRenderScene::GetRayTraceScene()
{
	return m_RayTraceScene;
}

IKVirtualGeometryScenePtr KRenderScene::GetVirtualGeometryScene()
{
	return m_VGScene;
}