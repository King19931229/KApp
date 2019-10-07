#pragma once
#include "glm/vec3.hpp"
#include <limits>

class KAABBBox
{
protected:
	enum ExtendMode
	{
		EM_DEFAULF,
		EM_NULL,
		EM_INFINITE
	};

	glm::vec3 m_Min;
	glm::vec3 m_Max;
	ExtendMode m_Mode;
public:
	KAABBBox()
	{
		m_Min = glm::vec3(std::numeric_limits<float>::max());
		m_Max = glm::vec3(std::numeric_limits<float>::min());
		m_Mode = EM_NULL;
	}

	inline bool SetInfinite() { m_Mode = EM_INFINITE; }
	inline bool SetNull() { m_Mode = EM_NULL; }
	inline bool IsInfinite() const { return m_Mode == EM_INFINITE; }
	inline bool IsNull() const { return  m_Mode == EM_NULL; }

	bool SetExtend(const glm::vec3& center, const glm::vec3& extend)
	{
		m_Min = center - extend;
		m_Max = center + extend;
		m_Mode = EM_DEFAULF;
	}

	bool Merge(const KAABBBox& other, KAABBBox* pResult)
	{
		if(other.IsInfinite() || IsInfinite())
		{
			if(pResult)
			{
				pResult->SetInfinite();
			}
			return true;
		}
		else if(other.IsNull() || IsNull())
		{
			if(pResult)
			{
				pResult->SetNull();
			}
			return true;
		}
		else
		{
			static auto MIN = [](float a, float b) { return a < b ? a : b; };
			static auto MAX = [](float a, float b) { return a > b ? a : b; };

			float max[3];
			float min[3];

			min[0] = MAX(m_Min.x, other.m_Min.x);
			max[0] = MIN(m_Max.x, other.m_Max.x);

			if(min[0] > max[0])
				return false;

			min[1] = MAX(m_Min.y, other.m_Min.y);
			max[1] = MIN(m_Max.y, other.m_Max.y);

			if(min[1] > max[1])
				return false;

			min[2] = MAX(m_Min.z, other.m_Min.z);
			max[2] = MIN(m_Max.z, other.m_Max.z);

			if(min[2] > max[2])
				return false;

			if(pResult)
			{
				pResult->m_Mode = EM_DEFAULF;
				pResult->m_Min = glm::vec3(min[0], min[1], min[2]);
				pResult->m_Max = glm::vec3(max[0], max[1], max[2]);
			}
			return true;
		}
	}
};