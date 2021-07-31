#pragma once
#include "Internal/KVertexDefinition.h"
#include "Internal/ECS/Component/KTransformComponent.h"
#include "Internal/ECS/Component/KRenderComponent.h"
#include "KBase/Interface/Entity/IKEntity.h"

struct KInstancePreparer
{
	struct InstanceGroup
	{
		KRenderComponent* render;
		std::vector<KConstantDefinition::OBJECT> instance;

		InstanceGroup()
		{
			render = nullptr;
		}
	};

	typedef std::shared_ptr<InstanceGroup> InstanceGroupPtr;
	typedef std::unordered_map<KMeshPtr, InstanceGroupPtr> MeshGroups;

	static void CalculateByMesh(const std::vector<KRenderComponent*>& cullRes, MeshGroups& meshGroups)
	{
		for (KRenderComponent* component : cullRes)
		{
			IKEntity* entity = component->GetEntityHandle();
			KTransformComponent* transform = nullptr;
			KRenderComponent* render = nullptr;
			if (entity->GetComponent(CT_TRANSFORM, (IKComponentBase**)&transform) && entity->GetComponent(CT_RENDER, (IKComponentBase**)&render))
			{
				InstanceGroupPtr instanceGroup = nullptr;
				KMeshPtr mesh = component->GetMesh();

				auto it = meshGroups.find(mesh);
				if (it != meshGroups.end())
				{
					instanceGroup = it->second;
				}
				else
				{
					instanceGroup = std::make_shared<InstanceGroup>();
					meshGroups[mesh] = instanceGroup;
				}

				instanceGroup->render = render;
				instanceGroup->instance.push_back({ transform->GetFinal() });
			}
		}
	}
};