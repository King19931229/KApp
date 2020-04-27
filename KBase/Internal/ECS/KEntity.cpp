#include "KEntity.h"

#include "Interface/Component/IKDebugComponent.h"
#include "Interface/Component/IKRenderComponent.h"
#include "Interface/Component/IKTransformComponent.h"

#include <assert.h>

KEntity::KEntity(size_t id)
	: m_Id(id)
{
}

KEntity::~KEntity()
{
	assert(m_Components.empty() && "Components not empty");
}

bool KEntity::GetComponentBase(ComponentType type, IKComponentBase** pptr)
{
	if(pptr)
	{
		auto it = m_Components.find(type);
		if(it != m_Components.end())
		{
			*pptr = it->second;
			return true;
		}
		else
		{
			*pptr = nullptr;
			return false;
		}
	}
	return false;
}

bool KEntity::HasComponent(ComponentType type)
{
	auto it = m_Components.find(type);
	return it != m_Components.end();
}

bool KEntity::HasComponents(const ComponentTypeList& components)
{
	for(const ComponentType& type : components)
	{
		if(!HasComponent(type))
		{
			return false;
		}
	}
	return true;
}

bool KEntity::RegisterComponentBase(ComponentType type, IKComponentBase** pptr)
{
	auto it = m_Components.find(type);
	if(it == m_Components.end())
	{
		IKComponentBase* component = KECS::ComponentManager->Alloc(type);
		m_Components.insert(ComponentMap::value_type(type, component));
		component->RegisterEntityHandle(this);
		if(pptr)
		{
			*pptr = component;
		}
		return true;
	}
	else
	{
		return false;
	}
}

bool KEntity::UnRegisterComponent(ComponentType type)
{
	auto it = m_Components.find(type);
	if(it != m_Components.end())
	{
		IKComponentBase*& component = it->second;
		component->UnRegisterEntityHandle();
		m_Components.erase(it);
		KECS::ComponentManager->Free(component);
		return true;
	}
	else
	{
		return false;
	}
}

bool KEntity::UnRegisterAllComponent()
{
	for(auto it = m_Components.begin(), itEnd = m_Components.end(); it != itEnd; ++it)
	{
		IKComponentBase*& component = it->second;
		component->UnRegisterEntityHandle();
		KECS::ComponentManager->Free(component);
	}
	m_Components.clear();
	return true;
}

bool KEntity::GetTransform(glm::mat4& transform)
{
	IKTransformComponent* transformComponent = nullptr;
	if (GetComponent(CT_TRANSFORM, &transformComponent))
	{
		transform = transformComponent->GetFinal();
	}
	else
	{
		transform = glm::mat4(1.0f);
	}

	return true;
}

bool KEntity::SetTransform(const glm::mat4& transform)
{
	IKTransformComponent* transformComponent = nullptr;
	if (GetComponent(CT_TRANSFORM, &transformComponent))
	{
		transformComponent->SetFinal(transform);
		return true;
	}
	return false;
}

bool KEntity::GetBound(KAABBBox& bound)
{
	IKComponentBase* component = nullptr;
	IKRenderComponent* renderComponent = nullptr;
	IKTransformComponent* transformComponent = nullptr;

	if (GetComponent(CT_RENDER, &component))
	{
		renderComponent = (IKRenderComponent*)component;
	}
	if (GetComponent(CT_TRANSFORM, &component))
	{
		transformComponent = (IKTransformComponent*)component;
	}

	if (renderComponent)
	{
		KAABBBox localBound;
		if (renderComponent->GetLocalBound(localBound))
		{
			if (transformComponent)
			{
				const glm::mat4& world = transformComponent->GetFinal();
				localBound.Transform(world, bound);
			}
			else
			{
				bound = localBound;
			}
			return true;
		}
	}

	return false;
}

bool KEntity::Intersect(const glm::vec3& origin, const glm::vec3& dir, glm::vec3& result, const float* maxDistance)
{
	KAABBBox bound;

	glm::vec3 boxResult;
	if (GetBound(bound) && bound.Intersection(origin, dir, boxResult))
	{
		if (maxDistance && glm::dot(boxResult - origin, dir) > *maxDistance)
		{
			return false;
		}

		glm::vec3 localOrigin;
		glm::vec3 localDir;

		IKRenderComponent* renderComponent = nullptr;
		if (GetComponent(CT_RENDER, &renderComponent))
		{
			glm::mat4 model;
			glm::mat4 invModel;
			if (GetTransform(model))
			{
				glm::mat4 invModel = glm::inverse(model);

				localOrigin = invModel * glm::vec4(origin, 1.0f);
				localDir = invModel * glm::vec4(dir, 0.0f);
				localDir = glm::normalize(localDir);

				glm::vec3 hitResult;
				bool intersect = maxDistance ?
					renderComponent->CloestPick(localOrigin, localDir, hitResult) :
					renderComponent->Pick(localOrigin, localDir, hitResult);

				if (intersect)
				{
					float dotRes = 0.0f;
					dotRes = glm::dot(hitResult - localOrigin, localDir);
					glm::vec3 localPoint = localOrigin + dotRes * localDir;
					result = model * glm::vec4(localPoint, 1.0f);

					if (maxDistance)
					{
						float distance = glm::dot(result - origin, dir);
						return distance < *maxDistance;
					}
					else
					{
						return true;
					}
				}

			}
		}
	}
	return false;
}

const char* KEntity::msComponent = "component";
const char* KEntity::msComponentType = "type";

bool KEntity::Save(IKXMLElementPtr element)
{
	for (auto& pair : m_Components)
	{
		ComponentType componentType = pair.first;
		IKComponentBase* component = pair.second;

		IKXMLElementPtr componentEle = element->NewElement(msComponent);
		componentEle->SetAttribute(msComponentType, KECS::ComponentTypeToString(componentType));

		component->Save(componentEle);
	}
	return true;
}

bool KEntity::Load(IKXMLElementPtr element)
{
	UnRegisterAllComponent();

	IKXMLElementPtr componentEle = element->FirstChildElement(msComponent);
	while (componentEle && !componentEle->IsEmpty())
	{
		IKXMLAttributePtr attri = componentEle->FindAttribute(msComponentType);
		if (attri && !attri->IsEmpty())
		{
			ComponentType componentType = KECS::StringToComponentType(attri->Value().c_str());
			if (componentType != CT_UNKNOWN)
			{
				IKComponentBase* component = nullptr;
				if (RegisterComponentBase(componentType, &component))
				{
					component->Load(componentEle);
				}				
			}
		}
		componentEle = componentEle->NextSiblingElement(msComponent);
	}
	return true;
}