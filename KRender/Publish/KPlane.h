#pragma once
#include "glm/glm.hpp"

class KPlane
{
public:
	enum PlaneSide
	{
		PS_COPLANER = 0,
		PS_POSITIVE = 1,
		PS_NEGATIVE = 2,
		PS_SPAN = 3
	};
protected:
	glm::vec3 m_Normal;
	float m_Dist;
public:
	KPlane()
		: m_Normal(0.0f, 1.0f, 0.0f),
		m_Dist(0.0f)
	{

	}

	void Init(const glm::vec3& normal, float dist)
	{
		float len = glm::length(normal);
		m_Normal = normal / len;
		m_Dist = dist / len;
	}

	void Init(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c)
	{
		m_Normal = glm::cross(b - a, c - a);
		m_Dist = -glm::dot(a, m_Normal);
		float len = glm::length(m_Normal);
		m_Normal /= len;
		m_Dist /= len;
	}

	inline const glm::vec3& GetNormal() const { return m_Normal; }
	inline float GetDist() const { return m_Dist; }
	inline float GetDistance(const glm::vec3& p) const { return glm::dot(m_Normal, p) + m_Dist; }

	inline PlaneSide GetSide(const glm::vec3& center, const glm::vec3& halfSize) const
	{
		float dist = GetDistance(center);
		float maxDist = abs(m_Normal.x * halfSize.x) + abs(m_Normal.y * halfSize.y) + abs(m_Normal.z * halfSize.z);
		if(dist < -maxDist)
			return PS_NEGATIVE;
		else if(dist > maxDist)
			return PS_POSITIVE;
		return PS_SPAN;
	}
};