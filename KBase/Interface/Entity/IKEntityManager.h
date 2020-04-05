#pragma once
#include "KBase/Interface/Entity/IKEntity.h"

struct IKEntityManager
{
	virtual ~IKEntityManager() {}

	virtual IKEntityPtr CreateEntity() = 0;
	virtual bool ReleaseEntity(IKEntityPtr& entity) = 0;

	virtual void ViewEntity(const ComponentTypeList& components, KEntityViewFunc func) = 0;
	virtual void ViewAllEntity(KEntityViewFunc func) = 0;
};

typedef std::unique_ptr<IKEntityManager> IKEntityManagerPtr;

namespace KECS
{
	extern bool CreateEntityManager();
	extern bool DestroyEntityManager();
	extern IKEntityManagerPtr EntityManager;
}