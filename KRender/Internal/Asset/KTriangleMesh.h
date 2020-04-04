#pragma once
#include "KBase/Publish/KTriangle.h"

struct KTriangleMesh
{
	std::vector<KTriangle> triangles;

	bool Pick(const glm::vec3& origin, const glm::vec3& dir, glm::vec3& result) const
	{
		for (const KTriangle& triangle : triangles)
		{
			if (triangle.Intersection(origin, dir, result))
			{
				return true;
			}
		}

		return false;
	}

	bool CloestPickPoint(const glm::vec3& origin, const glm::vec3& dir, glm::vec3& result) const
	{
		bool intersect = false;
		float minDotRes = 0.0f;

		for (const KTriangle& triangle : triangles)
		{
			glm::vec3 intersectPoint;
			if (triangle.Intersection(origin, dir, intersectPoint))
			{
				float dotResult = glm::dot(intersectPoint - origin, dir);
				if (!intersect || dotResult < minDotRes)
				{
					result = intersectPoint;
					minDotRes = dotResult;
				}
				intersect = true;
			}
		}

		return intersect;
	}

	void Destroy()
	{
		triangles.clear();
		triangles.shrink_to_fit();
	}
};