#pragma once
#include "glm/glm.hpp"
#include <initializer_list>
#include "KPlane.h"

class KTriangle
{
protected:
	glm::vec3 m_Points[3];
public:
	KTriangle()
	{
	}

	void Init(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2)
	{
		m_Points[0] = p0;
		m_Points[1] = p1;
		m_Points[2] = p2;
	}

	void Init(const glm::vec3 points[3])
	{
		m_Points[0] = points[0];
		m_Points[1] = points[1];
		m_Points[2] = points[2];
	}

	void Init(const std::initializer_list<glm::vec3> points)
	{
		assert(points.size() == 3);

		size_t idx = 0;
		for (const glm::vec3& point : points)
		{
			if (idx < 3)
			{
				m_Points[idx] = point;
			}
			++idx;
		}
	}

	bool IsPointInside(const glm::vec3& point)
	{
		glm::vec3 A0 = point - m_Points[0];
		glm::vec3 B0 = m_Points[1] - m_Points[0];
		glm::vec3 cross0 = glm::cross(A0, B0);

		glm::vec3 A1 = point - m_Points[1];
		glm::vec3 B1 = m_Points[2] - m_Points[1];
		glm::vec3 cross1 = glm::cross(A1, B1);

		glm::vec3 A2 = point - m_Points[2];
		glm::vec3 B2 = m_Points[0] - m_Points[2];
		glm::vec3 cross2 = glm::cross(A2, B2);

		if(glm::dot(cross0, cross1) > 0.0f && glm::dot(cross0, cross2) > 0.0f)
			return true;
		else
			return false;
	}

	bool Intersect(const glm::vec3& origin, const glm::vec3& dir)
	{
		KPlane plane;
		plane.Init(m_Points[0], m_Points[1], m_Points[2]);

		glm::vec3 intersectPoint;
		if (plane.Intersect(origin, dir, intersectPoint))
		{
			if (IsPointInside(intersectPoint))
			{
				return true;
			}
		}

		return false;
	}
};