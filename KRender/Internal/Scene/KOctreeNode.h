#pragma once
#include "KBase/Interface/Entity/IKEntity.h"
#include "KBase/Interface/Entity/IKEntityManager.h"
#include "Internal/ECS/Component/KRenderComponent.h"
#include "Internal/ECS/KECSGlobal.h"
#include "Publish/KCamera.h"
#include "glm/glm.hpp"
#include <deque>
#include <unordered_map>

// An object in the octree
struct KOctreeObject
{
	IKEntity* obj;
	KAABBBox bounds;
};

struct KOctreeNode;
typedef std::unordered_map<IKEntity*, KOctreeNode*> KEntityToNodeMap;

// A node in a BoundsOctree
// Copyright 2014 Nition, BSD licence (see LICENCE file). www.momentstudio.co.nz
struct KOctreeNode
{
private:
	// Centre of this node
	glm::vec3 center;
	// Length of this node if it has a looseness of 1.0
	float baseLength = 0;
	// Looseness value for this node
	float looseness = 0;
	// Minimum size for a node in this octree
	float minSize = 0;
	// Actual length of sides, taking the looseness value into account
	float adjLength = 0;
	// Bounding box that represents this node
	KAABBBox bounds;
	// Objects in this node
	std::vector<KOctreeObject> objects;
	// If there are already NUM_OBJECTS_ALLOWED in a node, we split it into children
	// A generally good number seems to be something around 8-15
	constexpr static int NUM_OBJECTS_ALLOWED = 8;
	// Child nodes, if any
	KOctreeNode* children = nullptr;
	// Bounds of potential children to this node. These are actual size (with looseness taken into account), not base size
	KAABBBox* childBounds = nullptr;
	// Record the mapping record for the entity to the node
	KEntityToNodeMap* sharedEntityToNode = nullptr;

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
		assert(children);
		for (auto i = 0; i < 8; ++i)
		{
			auto &curChild = children[i];
			auto numObjects = curChild.objects.size();
			for (auto it = curChild.objects.cbegin(), itEnd = curChild.objects.cend();
				it != itEnd; ++it)
			{
				const auto& curObj = *it;
				objects.push_back(curObj);

				(*sharedEntityToNode)[curObj.obj] = this;
			}
		}
		SAFE_DELETE_ARRAY(children);
	}

	void SetValues(float baseLengthVal, float minSizeVal, float loosenessVal, const glm::vec3& centerVal, KEntityToNodeMap* entityToNode)
	{
		assert(entityToNode);

		baseLength = baseLengthVal;
		minSize = minSizeVal;
		looseness = loosenessVal;
		center = centerVal;
		adjLength = looseness * baseLengthVal;
		sharedEntityToNode = entityToNode;

		// Create the bounding box.
		glm::vec3 size = glm::vec3(adjLength, adjLength, adjLength);
		bounds.InitFromHalfExtent(center, size);

		float quarter = baseLength / 4.0f;
		float childActualLength = (quarter / 2) * looseness;
		glm::vec3 childActualSize = glm::vec3(childActualLength, childActualLength, childActualLength);

		assert(!childBounds);
		childBounds = KNEW KAABBBox[8];

		childBounds[0].InitFromHalfExtent(center + quarter * glm::vec3(-1,  1, -1), childActualSize);
		childBounds[1].InitFromHalfExtent(center + quarter * glm::vec3( 1,  1, -1), childActualSize);
		childBounds[2].InitFromHalfExtent(center + quarter * glm::vec3(-1,  1,  1), childActualSize);
		childBounds[3].InitFromHalfExtent(center + quarter * glm::vec3( 1,  1,  1), childActualSize);
		childBounds[4].InitFromHalfExtent(center + quarter * glm::vec3(-1, -1, -1), childActualSize);
		childBounds[5].InitFromHalfExtent(center + quarter * glm::vec3( 1, -1, -1), childActualSize);
		childBounds[6].InitFromHalfExtent(center + quarter * glm::vec3(-1, -1,  1), childActualSize);
		childBounds[7].InitFromHalfExtent(center + quarter * glm::vec3( 1, -1,  1), childActualSize);
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
		children = KNEW KOctreeNode[8];
		children[0].SetValues(newLength, minSize, looseness, center + glm::vec3(-quarter, quarter, -quarter), sharedEntityToNode);
		children[1].SetValues(newLength, minSize, looseness, center + glm::vec3(quarter, quarter, -quarter), sharedEntityToNode);
		children[2].SetValues(newLength, minSize, looseness, center + glm::vec3(-quarter, quarter, quarter), sharedEntityToNode);
		children[3].SetValues(newLength, minSize, looseness, center + glm::vec3(quarter, quarter, quarter), sharedEntityToNode);
		children[4].SetValues(newLength, minSize, looseness, center + glm::vec3(-quarter, -quarter, -quarter), sharedEntityToNode);
		children[5].SetValues(newLength, minSize, looseness, center + glm::vec3(quarter, -quarter, -quarter), sharedEntityToNode);
		children[6].SetValues(newLength, minSize, looseness, center + glm::vec3(-quarter, -quarter, quarter), sharedEntityToNode);
		children[7].SetValues(newLength, minSize, looseness, center + glm::vec3(quarter, -quarter, quarter), sharedEntityToNode);
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

	void SubAdd(IKEntity* obj, const KAABBBox& objBounds)
	{
		if (!HasChildren())
		{
			if (objects.size() < NUM_OBJECTS_ALLOWED || (baseLength / 2.0f) < minSize)
			{
				KOctreeObject newObject = { obj, objBounds };
				objects.push_back(std::move(newObject));
				(*sharedEntityToNode)[obj] = this;
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
					bestFitChild = BestFitChild(existingObj.bounds.GetCenter());
					if (Encapsulates(children[bestFitChild].bounds, existingObj.bounds))
					{
						children[bestFitChild].SubAdd(existingObj.obj, existingObj.bounds);
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
			(*sharedEntityToNode)[obj] = this;
		}
	}

	// 用于 KNEW []
	KOctreeNode() {}
	// 禁止拷贝
	KOctreeNode(const KOctreeNode& rhs) = delete;
	KOctreeNode& operator=(const KOctreeNode& rhs) = delete;
public:
	KOctreeNode(float baseLengthVal, float minSizeVal, float loosenessVal, const glm::vec3& centerVal, KEntityToNodeMap* entityToNode)
	{
		SetValues(baseLengthVal, minSizeVal, loosenessVal, centerVal, entityToNode);
	}

	~KOctreeNode()
	{
		SAFE_DELETE_ARRAY(children);
		SAFE_DELETE_ARRAY(childBounds);
	}

	bool Add(IKEntity* obj, const KAABBBox& objBounds)
	{
		if (!Encapsulates(bounds, objBounds))
		{
			return false;
		}
		SubAdd(obj, objBounds);
		return true;
	}

	bool Remove(IKEntity* obj)
	{
		bool removed = false;

		for (auto it = objects.cbegin(), itEnd = objects.cend(); it != itEnd; ++it)
		{
			const auto& existingObj = *it;
			if (existingObj.obj == obj)
			{
				objects.erase(it);
				(*sharedEntityToNode).erase(obj);
				removed = true;
				break;
			}
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

	template<typename QueryResultType>
	void GetAll(QueryResultType& result)
	{
		for (auto it = objects.cbegin(), itEnd = objects.cend(); it != itEnd; ++it)
		{
			const auto& curObject = *it;
			result.push_back(curObject.obj);
		}

		if (children)
		{
			for (auto i = 0; i < 8; i++)
			{
				children[i].GetAll(result);
			}
		}
	}

	bool IsColliding(const KAABBBox& checkBounds)
	{
		if (!bounds.Intersect(checkBounds))
		{
			return false;
		}

		for (const auto& curObject : objects)
		{
			if (curObject.bounds.Intersect(checkBounds))
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
	void GetColliding(const glm::vec3& origin, const glm::vec3& dir, QueryResultType& result)
	{
		if (!bounds.Intersect(origin, dir))
		{
			return;
		}

		for (auto it = objects.cbegin(), itEnd = objects.cend(); it != itEnd; ++it)
		{
			const auto& curObject = *it;
			if (curObject.bounds.Intersect(origin, dir))
			{
				result.push_back(curObject.obj);
			}
		}

		if (children)
		{
			for (auto i = 0; i < 8; i++)
			{
				children[i].GetColliding(origin, dir, result);
			}
		}
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
			if (curObject.bounds.Intersect(checkBounds))
			{
				result.push_back(curObject.obj);
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
			if (camera.CheckVisible(curObject.bounds))
			{
				result.push_back(curObject.obj);
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

	bool GetObjectBound(KAABBBox& bound)
	{
		KAABBBox result;

		for (auto it = objects.cbegin(), itEnd = objects.cend(); it != itEnd; ++it)
		{
			const auto& curObject = *it;
			result = result.Merge(curObject.bounds);
		}

		if (children)
		{
			for (auto i = 0; i < 8; i++)
			{
				KAABBBox childResult;
				children[i].GetObjectBound(childResult);
				result = result.Merge(childResult);
			}
		}

		bound = result;

		return true;
	}
};
