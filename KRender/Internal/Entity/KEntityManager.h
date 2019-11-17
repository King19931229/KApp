#pragma once
#include "KEntity.h"

class KEntityManager
{
protected:
	typedef std::map<size_t, KEntityPtr> EntityMap;
	EntityMap m_Entities;
	size_t m_EntityHandleCounter;

	size_t GetAvailibleID();
public:
	KEntityManager();
	~KEntityManager();

	KEntityPtr CreateEntity();
	bool ReleaseEntity(KEntityPtr& entity);

	void ViewEntity(const ComponentTypeList& components, KEntityViewFunc func);
	void ViewAllEntity(KEntityViewFunc func);
};