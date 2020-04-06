#pragma once
#include "Interface/Entity/IKEntityManager.h"
#include <unordered_map>

class KEntityManager : public IKEntityManager
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

	IKEntityPtr CreateEntity() override;
	bool ReleaseEntity(IKEntityPtr& entity) override;

	void ViewEntity(const ComponentTypeList& components, KEntityViewFunc func) override;
	void ViewAllEntity(KEntityViewFunc func) override;
};