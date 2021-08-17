#include "KEntity.h"

#include "Interface/Component/IKDebugComponent.h"
#include "Interface/Component/IKRenderComponent.h"
#include "Interface/Component/IKTransformComponent.h"

#include <assert.h>

RTTR_REGISTRATION
{
#define KRTTR_REG_CLASS_NAME KEntity
#define KRTTR_REG_CLASS_NAME_STR "Entity"

	KRTTR_REG_CLASS_BEGIN()
	KRTTR_REG_PROPERTY_CUSTOM_NAME(name, m_Name, MDT_STDSTRING)
	KRTTR_REG_PROPERTY_READ_ONLY(render, GetRenderComponent, MDT_OBJECT)
	KRTTR_REG_PROPERTY_READ_ONLY(debug, GetDebugComponent, MDT_OBJECT)
	KRTTR_REG_PROPERTY_READ_ONLY(transform, GetTransformComponent, MDT_OBJECT)
	KRTTR_REG_CLASS_END()

#undef KRTTR_REG_CLASS_NAME_STR
#undef KRTTR_REG_CLASS_NAME
}

KEntity::KEntity()
	: m_Id(0),
	m_Name("Unnamed")
{
	ZERO_ARRAY_MEMORY(m_Components);
}

KEntity::KEntity(size_t id)
	: m_Id(id),
	m_Name("Unnamed")
{
	ZERO_ARRAY_MEMORY(m_Components);
}

KEntity::~KEntity()
{
	for (uint32_t i = 0; i < CT_COUNT; ++i)
	{
		ASSERT_RESULT(!m_Components[i]);
	}
}

bool KEntity::GetComponentBase(ComponentType type, IKComponentBase** pptr)
{
	if(pptr)
	{
		if (type != CT_UNKNOWN && m_Components[type])
		{
			*pptr = m_Components[type];
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
	if (type != CT_UNKNOWN && m_Components[type])
	{
		return true;
	}
	return false;
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
	if (type != CT_UNKNOWN && !m_Components[type])
	{
		IKComponentBase* component = KECS::ComponentManager->Alloc(type);
		m_Components[type] = component;
		component->RegisterEntityHandle(this);
		if (pptr)
		{
			*pptr = component;
		}
		return true;
	}
	return false;
}

bool KEntity::UnRegisterComponent(ComponentType type)
{
	if (type != CT_UNKNOWN && m_Components[type])
	{
		IKComponentBase* component = m_Components[type];
		component->UnRegisterEntityHandle();
		KECS::ComponentManager->Free(component);
		m_Components[type] = nullptr;
		return true;
	}

	return false;
}

bool KEntity::UnRegisterAllComponent()
{
	for (uint32_t i = 0; i < CT_COUNT; ++i)
	{
		IKComponentBase* component = m_Components[i];
		if (component)
		{
			component->UnRegisterEntityHandle();
			KECS::ComponentManager->Free(component);
			m_Components[i] = nullptr;
		}
	}

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

bool KEntity::PreTick()
{
	for (uint32_t i = 0; i < CT_COUNT; ++i)
	{
		IKComponentBase* component = m_Components[i];
		if (component)
		{
			component->PreTick();
		}
	}
	return true;
}

bool KEntity::PostTick()
{
	for (uint32_t i = 0; i < CT_COUNT; ++i)
	{
		IKComponentBase* component = m_Components[i];
		if (component)
		{
			component->PostTick();
		}
	}
	return true;
}

const char* KEntity::msName = "name";
const char* KEntity::msComponent = "component";
const char* KEntity::msComponentType = "type";

bool KEntity::Save(IKXMLElementPtr element)
{
	for (uint32_t i = 0; i < CT_COUNT; ++i)
	{
		IKComponentBase* component = m_Components[i];
		if (component)
		{
			ComponentType componentType = static_cast<ComponentType>(i);
			IKXMLElementPtr componentEle = element->NewElement(msComponent);
			componentEle->SetAttribute(msComponentType, KECS::ComponentTypeToString(componentType));
			component->Save(componentEle);
		}
	}
	element->SetAttribute(msName, m_Name.c_str());

	return true;
}

bool KEntity::Load(IKXMLElementPtr element)
{
	UnRegisterAllComponent();

	IKXMLAttributePtr attri = element->FindAttribute(msName);
	if (attri && !attri->IsEmpty())
	{
		m_Name = attri->Value();
	}

	IKXMLElementPtr componentEle = element->FirstChildElement(msComponent);
	while (componentEle && !componentEle->IsEmpty())
	{
		attri = componentEle->FindAttribute(msComponentType);
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