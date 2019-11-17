#include "KEntityManager.h"
#include <assert.h>

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
	// TODO
	return m_EntityHandleCounter++;
}

KEntityPtr KEntityManager::CreateEntity()
{
	size_t id = GetAvailibleID();
	KEntity* newEntity = new KEntity(id);
	KEntityPtr entity = KEntityPtr(newEntity);

	assert(m_Entities.find(id) == m_Entities.end());
	m_Entities[id] = entity;
	return entity;
}

bool KEntityManager::ReleaseEntity(KEntityPtr& entity)
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
		KEntityPtr& entity = pair.second;
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
		KEntityPtr& entity = pair.second;
		func(entity);
	}
}