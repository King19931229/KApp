#pragma once
#include "Internal/ECS/KEntity.h"
#include "Internal/ECS/Component/KRenderComponent.h"
#include "Internal/ECS/KECSGlobal.h"
#include "Publish/KCamera.h"
#include "glm/glm.hpp"
#include <deque>

// An object in the octree
struct KOctreeObject
{
	KEntity* Obj;
	KAABBBox Bounds;
};

// A node in a BoundsOctree
// Copyright 2014 Nition, BSD licence (see LICENCE file). www.momentstudio.co.nz
struct KOctreeNode
{
private:
	// Centre of this node
	glm::vec3 center;
	// Length of this node if it has a looseness of 1.0
	float baseLength;
	// Looseness value for this node
	float looseness;
	// Minimum size for a node in this octree
	float minSize;
	// Actual length of sides, taking the looseness value into account
	float adjLength;
	// Bounding box that represents this node
	KAABBBox bounds;
	// Objects in this node
	std::deque<KOctreeObject> objects;
	// If there are already NUM_OBJECTS_ALLOWED in a node, we split it into children
	// A generally good number seems to be something around 8-15
	constexpr static int NUM_OBJECTS_ALLOWED = 8;
	// Child nodes, if any
	KOctreeNode* children = nullptr;
	// Bounds of potential children to this node. These are actual size (with looseness taken into account), not base size
	KAABBBox* childBounds = nullptr;
	// Render component for debugging info
	KEntityPtr boundEntity = nullptr;

	bool HasChildren()
	{
		return children != nullptr;
	}

	bool ShouldMerge()
	{
		auto totlaObjects = objects.size();
		if (children)
		{
			for (auto i = 0; i < 8; ++i)
			{
				if (children[i].children != nullptr)
				{
					return false;
				}
				totlaObjects += children[i].objects.size();
			}
		}
		return totlaObjects <= NUM_OBJECTS_ALLOWED;
	}

	void Merge()
	{
		for (auto i = 0; i < 8; ++i)
		{
			auto curChild = children[i];
			auto numObjects = curChild.objects.size();
			for (auto it = curChild.objects.cbegin(), itEnd = curChild.objects.cend();
				it != itEnd; ++it)
			{
				const auto& curObj = *it;
				objects.push_back(curObj);
			}
		}
		SAFE_DELETE_ARRAY(children);
	}

	void SetValues(float baseLengthVal, float minSizeVal, float loosenessVal, const glm::vec3& centerVal)
	{
		baseLength = baseLengthVal;
		minSize = minSizeVal;
		looseness = loosenessVal;
		center = centerVal;
		adjLength = looseness * baseLengthVal;

		// Create the bounding box.
		glm::vec3 size = glm::vec3(adjLength, adjLength, adjLength);
		bounds.InitFromExtent(center, size);

		float quarter = baseLength / 4.0f;
		float childActualLength = (quarter / 2) * looseness;
		glm::vec3 childActualSize = glm::vec3(childActualLength, childActualLength, childActualLength);

		childBounds = new KAABBBox[8];
		childBounds[0].InitFromExtent(center + glm::vec3(-quarter, quarter, -quarter), childActualSize);
		childBounds[1].InitFromExtent(center + glm::vec3(quarter, quarter, -quarter), childActualSize);
		childBounds[2].InitFromExtent(center + glm::vec3(-quarter, quarter, quarter), childActualSize);
		childBounds[3].InitFromExtent(center + glm::vec3(quarter, quarter, quarter), childActualSize);
		childBounds[4].InitFromExtent(center + glm::vec3(-quarter, -quarter, -quarter), childActualSize);
		childBounds[5].InitFromExtent(center + glm::vec3(quarter, -quarter, -quarter), childActualSize);
		childBounds[6].InitFromExtent(center + glm::vec3(-quarter, -quarter, quarter), childActualSize);
		childBounds[7].InitFromExtent(center + glm::vec3(quarter, -quarter, quarter), childActualSize);
	}

	static bool Encapsulates(const KAABBBox& outerBounds, const KAABBBox& innerBounds)
	{
		return outerBounds.Intersect(innerBounds.GetMin()) && outerBounds.Intersect(innerBounds.GetMax());
	}

	void Split()
	{
		float quarter = baseLength / 4.0f;
		float newLength = baseLength / 2.0f;
		assert(!children);
		children = new KOctreeNode[8];
		children[0].SetValues(newLength, minSize, looseness, center + glm::vec3(-quarter, quarter, -quarter));
		children[1].SetValues(newLength, minSize, looseness, center + glm::vec3(quarter, quarter, -quarter));
		children[2].SetValues(newLength, minSize, looseness, center + glm::vec3(-quarter, quarter, quarter));
		children[3].SetValues(newLength, minSize, looseness, center + glm::vec3(quarter, quarter, quarter));
		children[4].SetValues(newLength, minSize, looseness, center + glm::vec3(-quarter, -quarter, -quarter));
		children[5].SetValues(newLength, minSize, looseness, center + glm::vec3(quarter, -quarter, -quarter));
		children[6].SetValues(newLength, minSize, looseness, center + glm::vec3(-quarter, -quarter, quarter));
		children[7].SetValues(newLength, minSize, looseness, center + glm::vec3(quarter, -quarter, quarter));
	}

	inline int BestFitChild(const glm::vec3& objBoundsCenter)
	{
		return (objBoundsCenter.x <= center.x ? 0 : 1) + (objBoundsCenter.y >= center.y ? 0 : 4) + (objBoundsCenter.z <= center.z ? 0 : 2);
	}

	bool HasAnyObjects()
	{
		if (!objects.empty())
		{
			return true;
		}
		if (children)
		{
			for (auto i = 0; i < 8; ++i)
			{
				if (children[i].HasAnyObjects())
				{
					return true;
				}
			}
		}
		return false;
	}

	void SubAdd(KEntity* obj, const KAABBBox& objBounds)
	{
		if (!HasChildren())
		{
			if (objects.size() < NUM_OBJECTS_ALLOWED || (baseLength / 2.0f) < minSize)
			{
				KOctreeObject newObject = { obj, objBounds };
				objects.push_back(std::move(newObject));
				return;
			}

			int bestFitChild = -1;
			if (!children)
			{
				Split();
				assert(children);

				for (auto it = objects.cbegin(); it != objects.cend();)
				{
					const auto& existingObj = *it;
					bestFitChild = BestFitChild(existingObj.Bounds.GetCenter());
					if (Encapsulates(children[bestFitChild].bounds, existingObj.Bounds))
					{
						children[bestFitChild].SubAdd(existingObj.Obj, existingObj.Bounds);
						it = objects.erase(it);
					}
					else
					{
						++it;
					}
				}
			}
		}

		int bestFit = BestFitChild(objBounds.GetCenter());
		if (Encapsulates(children[bestFit].bounds, objBounds))
		{
			children[bestFit].SubAdd(obj, objBounds);
		}
		else
		{
			KOctreeObject newObj = { obj, objBounds };
			objects.push_back(std::move(newObj));
		}
	}

	bool SubRemove(KEntity* obj, const KAABBBox& objBounds)
	{
		bool removed = false;

		for (auto it = objects.cbegin(), itEnd = objects.cend(); it != itEnd; ++it)
		{
			const auto& existingObj = *it;
			if (existingObj.Obj == obj)
			{
				objects.erase(it);
				removed = true;
				break;
			}
		}

		if (!removed && children)
		{
			int bestFitChild = BestFitChild(objBounds.GetCenter());
			removed = children[bestFitChild].SubRemove(obj, objBounds);
		}

		if (removed && children)
		{
			if (ShouldMerge())
			{
				Merge();
			}
		}

		return removed;
	}

	KOctreeNode()
	{
	}
public:
	KOctreeNode(float baseLengthVal, float minSizeVal, float loosenessVal, const glm::vec3& centerVal)
	{
		SetValues(baseLengthVal, minSizeVal, loosenessVal, centerVal);
	}

	~KOctreeNode()
	{
		SAFE_DELETE_ARRAY(children);
		SAFE_DELETE_ARRAY(childBounds);
		if (boundEntity)
		{
			boundEntity->UnRegisterAllComponent();
			KECSGlobal::EntityManager.ReleaseEntity(boundEntity);
			boundEntity = nullptr;
		}
	}

	bool Add(KEntity* obj, const KAABBBox& objBounds)
	{
		if (!Encapsulates(bounds, objBounds))
		{
			return false;
		}
		SubAdd(obj, objBounds);
		return true;
	}

	bool Remove(KEntity* obj, const KAABBBox& objBounds)
	{
		if (!Encapsulates(bounds, objBounds))
		{
			return false;
		}
		return SubRemove(obj, objBounds);
	}

	bool IsColliding(const KAABBBox& checkBounds)
	{
		if (!bounds.Intersect(checkBounds))
		{
			return false;
		}

		for (const auto& curObject : objects)
		{
			if (curObject.Bounds.Intersect(checkBounds))
			{
				return true;
			}
		}

		if (children)
		{
			for (auto i = 0; i < 8; i++)
			{
				if (children[i].IsColliding(checkBounds))
				{
					return true;
				}
			}
		}

		return false;
	}

	template<typename QueryResultType>
	void GetColliding(const KAABBBox& checkBounds, QueryResultType& result)
	{
		if (!bounds.Intersect(checkBounds))
		{
			return;
		}

		for (auto it = objects.cbegin(), itEnd = objects.cend(); it != itEnd; ++it)
		{
			const auto& curObject = *it;
			if (curObject.Bounds.Intersect(checkBounds))
			{
				result.push_back(curObject.Obj);
			}
		}

		if (children)
		{
			for (auto i = 0; i < 8; i++)
			{
				children[i].GetColliding(checkBounds, result);
			}
		}
	}

	template<typename QueryResultType>
	void GetWithinCamera(const KCamera& camera, QueryResultType& result)
	{
		if (!camera.CheckVisible(bounds))
		{
			return;
		}

		for (auto it = objects.cbegin(), itEnd = objects.cend(); it != itEnd; ++it)
		{
			const auto& curObject = *it;
			if (camera.CheckVisible(curObject.Bounds))
			{
				result.push_back(curObject.Obj);
			}
		}

		if (children)
		{
			for (auto i = 0; i < 8; i++)
			{
				children[i].GetWithinCamera(camera, result);
			}
		}
	}

	template<typename Type>
	void GetDebugRender(Type& componentList)
	{
		if (!boundEntity)
		{
			boundEntity = KECSGlobal::EntityManager.CreateEntity();

			KComponentBase* component = nullptr;
			if (boundEntity->RegisterComponent(CT_RENDER, &component))
			{
				((KRenderComponent*)component)->InitAsBox(bounds);
			}

			if (boundEntity->RegisterComponent(CT_TRANSFORM, &component))
			{
				((KTransformComponent*)component)->SetPosition(bounds.GetCenter());
			}
		}

		KRenderComponent* renderComponent = nullptr;
		if (boundEntity->GetComponent(CT_RENDER, (KComponentBase**)&renderComponent))
		{
			componentList.push_back(renderComponent);
		}

		if (children)
		{
			for (auto i = 0; i < 8; i++)
			{
				children[i].GetDebugRender(componentList);
			}
		}
	}
};