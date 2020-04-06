#include "KEntityManager.h"
#include "KEntity.h"

#include <assert.h>

namespace KECS
{
	IKEntityManagerPtr EntityManager = nullptr;

	bool CreateEntityManager()
	{
		assert(!EntityManager);
		if (!EntityManager)
		{
			EntityManager = IKEntityManagerPtr(new KEntityManager());
			((KEntityManager*)EntityManager.get())->Init();
			return true;
		}
		return false;
	}

	bool DestroyEntityManager()
	{
		if (EntityManager)
		{
			((KEntityManager*)EntityManager.get())->UnInit();
			EntityManager = nullptr;
			return true;
		}
		return false;
	}
}

KEntityManager::KEntityManager()
	: m_EntityHandleCounter(0)
{

}

KEntityManager::~KEntityManager()
{
	assert(m_Entities.empty() && "entities is not empty");
}

size_t KEntityManager::GetAvailibleID()
{
	return m_EntityHandleCounter++;
}

void KEntityManager::Init()
{
}

void KEntityManager::UnInit()
{
	for(auto pair : m_Entities)
	{
		IKEntityPtr& entity = pair.second;
		entity->UnRegisterAllComponent();
	}
	m_Entities.clear();
}

IKEntityPtr KEntityManager::CreateEntity()
{
	size_t id = GetAvailibleID();
	IKEntityPtr entity = IKEntityPtr(new KEntity(id));
	assert(m_Entities.find(id) == m_Entities.end());
	m_Entities[id] = entity;
	return entity;
}

bool KEntityManager::ReleaseEntity(IKEntityPtr& entity)
{
	if(entity != nullptr)
	{
		size_t id = entity->GetID();
		auto it = m_Entities.find(id);
		if(it != m_Entities.end())
		{
			m_Entities.erase(id);
			entity = nullptr;
			return true;
		}
		else
		{
			return false;
		}
	}
	else
	{
		return false;
	}
}

void KEntityManager::ViewEntity(const ComponentTypeList& components, KEntityViewFunc func)
{
	for(auto pair : m_Entities)
	{
		IKEntityPtr& entity = pair.second;
		if(entity->HasComponents(components))
		{
			func(entity);
		}
	}
}

void KEntityManager::ViewAllEntity(KEntityViewFunc func)
{
	for(auto pair : m_Entities)
	{
		IKEntityPtr& entity = pair.second;
		func(entity);
	}
}