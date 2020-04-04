#pragma once
#include "KEntity.h"

class KEntityManager
{
protected:
	typedef std::unordered_map<size_t, IKEntityPtr> EntityMap;
	EntityMap m_Entities;
	size_t m_EntityHandleCounter;

	size_t GetAvailibleID();
public:
	KEntityManager();
	~KEntityManager();

	void Init();
	void UnInit();

	IKEntityPtr CreateEntity();
	bool ReleaseEntity(IKEntityPtr& entity);

	void ViewEntity(const ComponentTypeList& components, KEntityViewFunc func);
	void ViewAllEntity(KEntityViewFunc func);
};