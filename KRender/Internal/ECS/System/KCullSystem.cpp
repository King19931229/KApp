#include "KCullSystem.h"
#include "Internal/ECS/KECSGlobal.h"
#include "KBase/Interface/Entity/IKEntityManager.h"

KCullSystem::KCullSystem()
{
}

KCullSystem::~KCullSystem()
{
}

void KCullSystem::Execute(const KCamera& camera, std::vector<KRenderComponent*>& result)
{
	KECS::EntityManager->ViewAllEntity([&](IKEntityPtr entity)
	{
		KRenderComponent* component = nullptr;
		if(entity->GetComponent(CT_RENDER, (IKComponentBase**)&component))
		{
			if(component && component->GetMesh())
			{
				// TODO BoundBox
				result.push_back(component);
			}
		}
	});
}