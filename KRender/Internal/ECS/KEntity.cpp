#include "KEntity.h"
#include "Component/KComponentBase.h"
#include "KECSGlobal.h"

#include <assert.h>

KEntity::KEntity(size_t id)
	: m_Id(id)
{
}

KEntity::~KEntity()
{
	assert(m_Components.empty() && "Components not empty");
}

bool KEntity::GetComponentBase(ComponentType type, KComponentBase** pptr)
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

bool KEntity::RegisterComponentBase(ComponentType type, KComponentBase** pptr)
{
	auto it = m_Components.find(type);
	if(it == m_Components.end())
	{
		KComponentBase* component = KECSGlobal::ComponentManager.Alloc(type);
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
		KComponentBase*& component = it->second;
		component->UnRegisterEntityHandle();
		m_Components.erase(it);
		KECSGlobal::ComponentManager.Free(component);
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
		KComponentBase*& component = it->second;
		component->UnRegisterEntityHandle();
		KECSGlobal::ComponentManager.Free(component);
	}
	m_Components.clear();
	return true;
}

bool KEntity::GetTransform(glm::mat4& transform)
{
	KTransformComponent* transformComponent = nullptr;
	if (GetComponent(CT_TRANSFORM, &transformComponent))
	{
		const auto& finalTransform = transformComponent->FinalTransform();
		transform = finalTransform.MODEL;
	}
	else
	{
		transform = glm::mat4(1.0f);
	}

	return true;
}

bool KEntity::GetBound(KAABBBox& bound)
{
	KComponentBase* component = nullptr;
	KRenderComponent* renderComponent = nullptr;
	KTransformComponent* transformComponent = nullptr;

	if (GetComponent(CT_RENDER, &component))
	{
		renderComponent = (KRenderComponent*)component;
	}
	if (GetComponent(CT_TRANSFORM, &component))
	{
		transformComponent = (KTransformComponent*)component;
	}

	if (renderComponent)
	{
		KMeshPtr mesh = renderComponent->GetMesh();
		if (mesh)
		{
			const KAABBBox& localBound = mesh->GetLocalBound();
			if (transformComponent)
			{
				const auto& finalTransform = transformComponent->FinalTransform();
				localBound.Transform(finalTransform.MODEL, bound);
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

		KRenderComponent* renderComponent = nullptr;
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

				KMeshPtr mesh = renderComponent->GetMesh();
				if (mesh)
				{
					const KTriangleMesh& triangleMesh = mesh->GetTriangleMesh();

					glm::vec3 triangleResult;
					bool intersect = false;

					if (maxDistance)
					{
						if (triangleMesh.CloestPickPoint(localOrigin, localDir, triangleResult))
						{
							intersect = true;
						}
					}
					else
					{
						glm::vec3 triangleResult;
						if (triangleMesh.Pick(localOrigin, localDir, triangleResult))
						{
							intersect = true;
						}
					}

					if (intersect)
					{
						float dotRes = 0.0f;
						dotRes = glm::dot(triangleResult - localOrigin, localDir);
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
	}
	return false;
}