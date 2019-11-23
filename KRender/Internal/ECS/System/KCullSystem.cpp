#include "KCullSystem.h"
#include "Internal/ECS/KECSGlobal.h"

KCullSystem::KCullSystem()
{
}

KCullSystem::~KCullSystem()
{
}

void KCullSystem::Execute(const KCamera& camera, std::vector<KRenderComponent*>& result)
{
	KECSGlobal::EntityManager.ViewAllEntity([&](KEntityPtr entity)
	{
		KRenderComponent* component = nullptr;
		if(entity->GetComponent(CT_RENDER, (KComponentBase**)&component))
		{
			if(component && component->GetMesh())
			{
				// TODO BoundBox
				result.push_back(component);
			}
		}
	});
}