#pragma once
#include "glm/glm.hpp"
#include <limits>
#include <algorithm>
#include <vector>

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
		m_Min = glm::vec3(0.0f);
		m_Max = glm::vec3(0.0f);
		m_Mode = EM_NULL;
	}

	inline void SetInfinite() { m_Mode = EM_INFINITE; }
	inline void SetNull() { m_Mode = EM_NULL; }

	inline bool IsInfinite() const { return m_Mode == EM_INFINITE; }
	inline bool IsNull() const { return  m_Mode == EM_NULL; }
	inline bool IsDefault() const { return m_Mode == EM_DEFAULF; }

	inline glm::vec3 GetCenter() const { return (m_Min + m_Max) * 0.5f; }
	inline glm::vec3 GetExtend() const { return m_Max - m_Min; }
	inline const glm::vec3& GetMin() const { return m_Min; }
	inline const glm::vec3& GetMax() const { return m_Max; }
	inline glm::vec3 GetMin() { return m_Min; }
	inline glm::vec3 GetMax() { return m_Max; }

	void InitFromHalfExtent(const glm::vec3& center, const glm::vec3& extend)
	{
		m_Min = center - extend;
		m_Max = center + extend;
		m_Mode = EM_DEFAULF;
	}

	void InitFromMinMax(const glm::vec3& min, const glm::vec3& max)
	{
		m_Min = min;
		m_Max = max;

		assert(m_Max.x >= m_Min.x);
		assert(m_Max.y >= m_Min.y);
		assert(m_Max.z >= m_Min.z);

		m_Mode = EM_DEFAULF;
	}

	KAABBBox Transform(const glm::mat4& transform) const
	{
		KAABBBox result;
		if (IsDefault())
		{
#if 1
			float min[3];
			float max[3];

			min[0] = max[0] = transform[3][0];
			min[1] = max[1] = transform[3][1];
			min[2] = max[2] = transform[3][2];

			float e = 0.0f, f = 0.0f;
			for(int i = 0; i < 3; i++)
			{
				for(int j = 0; j < 3; j++)
				{
					e = transform[j][i] * m_Min[i];
					f = transform[j][i] * m_Max[i];

					if(e < f)
					{
						min[j] += e;
						max[j] += f;
					}
					else
					{
						min[j] += f;
						max[j] += e;
					}
				}
			}

			result.m_Mode = EM_DEFAULF;

			result.m_Min[0] = min[0];
			result.m_Min[1] = min[1];
			result.m_Min[2] = min[2];

			result.m_Max[0] = max[0];
			result.m_Max[1] = max[1];
			result.m_Max[2] = max[2];
#else
			glm::vec3 min = glm::vec3(transform[3]);
			glm::vec3 max = min;
			glm::vec3 v0, v1;

			glm::vec3 thisMin = GetMin();
			glm::vec3 thisMax = GetMax();

			glm::vec3 right = glm::vec3(transform[0]);
			v0 = right * thisMin.x;
			v1 = right * thisMax.x;
			min += glm::min(v0, v1);
			max += glm::max(v0, v1);

			glm::vec3 up = glm::vec3(transform[1]);
			v0 = up * thisMin.y;
			v1 = up * thisMax.y;
			min += glm::min(v0, v1);
			max += glm::max(v0, v1);

			glm::vec3 back = glm::vec3(transform[2]);
			v0 = back * thisMin.z;
			v1 = back * thisMax.z;
			min += glm::min(v0, v1);
			max += glm::max(v0, v1);

			result.InitFromMinMax(min, max);
#endif
		}
		return result;
	}

	float DistanceSquare(const glm::vec3& point) const
	{
		float distSquare = 0;

		if (point.x > m_Max.x)
			distSquare += (point.x - m_Max.x) * (point.x - m_Max.x);
		if (point.x < m_Min.x)
			distSquare += (point.x - m_Min.x) * (point.x - m_Min.x);

		if (point.y > m_Max.y)
			distSquare += (point.y - m_Max.y) * (point.y - m_Max.y);
		if (point.y < m_Min.y)
			distSquare += (point.y - m_Min.y) * (point.y - m_Min.y);

		if (point.z > m_Max.z)
			distSquare += (point.z - m_Max.z) * (point.z - m_Max.z);
		if (point.z < m_Min.z)
			distSquare += (point.z - m_Min.z) * (point.z - m_Min.z);

		return distSquare;
	}

	inline float Distance(const glm::vec3& point) const
	{
		return sqrt(DistanceSquare(point));
	}

	inline float Surface() const
	{
		glm::vec3 extend = GetExtend();
		return 2 * (extend.x * extend.y + extend.x * extend.z + extend.y * extend.z);
	}

	float Volume() const
	{
		glm::vec3 extend = GetExtend();
		return extend.x * extend.y * extend.z;
	}

	bool Intersect(const glm::vec3& point) const
	{
		if(m_Max.x < point.x)
			return false;
		if(m_Max.y < point.y)
			return false;
		if(m_Max.z < point.z)
			return false;
		if(m_Min.x > point.x)
			return false;
		if(m_Min.y > point.y)
			return false;
		if(m_Min.z > point.z)
			return false;
		return true;
	}

	bool Intersect(const KAABBBox& other) const
	{
		if(m_Max.x < other.m_Min.x)
			return false;
		if(m_Max.y < other.m_Min.y)
			return false;
		if(m_Max.z < other.m_Min.z)
			return false;
		if(m_Min.x > other.m_Max.x)
			return false;
		if(m_Min.y > other.m_Max.y)
			return false;
		if(m_Min.z > other.m_Max.z)
			return false;
		return true;
	}

	bool Intersection(const KAABBBox& other, KAABBBox& result) const
	{
		if(other.IsInfinite() || IsInfinite())
		{
			result.SetInfinite();
			return true;
		}
		else if(other.IsNull() || IsNull())
		{
			result.SetNull();
			return true;
		}
		else
		{
			float max[3];
			float min[3];

			min[0] = std::max(m_Min.x, other.m_Min.x);
			max[0] = std::min(m_Max.x, other.m_Max.x);

			if(min[0] > max[0])
				return false;

			min[1] = std::max(m_Min.y, other.m_Min.y);
			max[1] = std::min(m_Max.y, other.m_Max.y);

			if(min[1] > max[1])
				return false;

			min[2] = std::max(m_Min.z, other.m_Min.z);
			max[2] = std::min(m_Max.z, other.m_Max.z);

			if(min[2] > max[2])
				return false;

			result.m_Mode = EM_DEFAULF;
			result.m_Min = glm::vec3(min[0], min[1], min[2]);
			result.m_Max = glm::vec3(max[0], max[1], max[2]);

			return true;
		}
	}

	KAABBBox Merge(const KAABBBox& other)
	{
		KAABBBox result;
		if (IsInfinite())
		{
			result.m_Mode = m_Mode;
			result.m_Min = m_Min;
			result.m_Max = m_Max;
		}
		else if (IsNull())
		{
			result.m_Mode = other.m_Mode;
			result.m_Min = other.m_Min;
			result.m_Max = other.m_Max;
		}
		else
		{
			if (other.IsInfinite())
			{
				result.m_Mode = other.m_Mode;
				result.m_Min = other.m_Min;
				result.m_Max = other.m_Max;
			}
			else if (other.IsNull())
			{
				result.m_Mode = m_Mode;
				result.m_Min = m_Min;
				result.m_Max = m_Max;
			}
			else
			{
				result.m_Mode = EM_DEFAULF;

				result.m_Max.x = std::max(m_Max.x, other.m_Max.x);
				result.m_Max.y = std::max(m_Max.y, other.m_Max.y);
				result.m_Max.z = std::max(m_Max.z, other.m_Max.z);

				result.m_Min.x = std::min(m_Min.x, other.m_Min.x);
				result.m_Min.y = std::min(m_Min.y, other.m_Min.y);
				result.m_Min.z = std::min(m_Min.z, other.m_Min.z);
			}
		}
		return result;
	}

	KAABBBox Merge(const glm::vec3& point)
	{
		KAABBBox result;
		if (IsInfinite())
		{
			result.m_Mode = EM_INFINITE;
			result.m_Min = m_Min;
			result.m_Max = m_Max;
		}
		else if (IsNull())
		{
			result.m_Mode = EM_DEFAULF;
			result.m_Min = result.m_Max = point;
		}
		else
		{
			result.m_Mode = EM_DEFAULF;

			result.m_Max.x = std::max(m_Max.x, point.x);
			result.m_Max.y = std::max(m_Max.y, point.y);
			result.m_Max.z = std::max(m_Max.z, point.z);

			result.m_Min.x = std::min(m_Min.x, point.x);
			result.m_Min.y = std::min(m_Min.y, point.y);
			result.m_Min.z = std::min(m_Min.z, point.z);
		}
		return result;
	}

	bool Intersect(const glm::vec3& origin, const glm::vec3& dir) const
	{
		glm::vec3 result;
		if (Intersection(origin, dir, result))
		{
			return true;
		}
		return false;
	}

	bool Intersection(const glm::vec3& origin, const glm::vec3& dir, glm::vec3& result) const
	{
		glm::vec3 inv_dir = 1.0f / dir;
		glm::vec3 tMin = (m_Min - origin) * inv_dir;
		glm::vec3 tMax = (m_Max - origin) * inv_dir;
		glm::vec3 t1 = glm::min(tMin, tMax);
		glm::vec3 t2 = glm::max(tMin, tMax);
		float tNear = std::max(std::max(t1.x, t1.y), t1.z);
		float tFar = std::min(std::min(t2.x, t2.y), t2.z);

		if (tNear <= tFar && tFar >= 0.0f)
		{
			if (tNear > 0.0f)
			{
				result = origin + dir * tNear;
			}
			else
			{
				result = origin;
			}
			return true;
		}

		return false;
	}

#if 0
      1------2
      /|    /|
     / |   / |
    5-----4  |
    |  0--|--3
    | /   | /
    |/    |/
    6-----7
#endif
	bool GetAllCorners(std::vector<glm::vec3>& results) const
	{
		if(IsDefault())
		{
			results.resize(8);

			results[0]		= m_Min;

			results[1].x	= m_Min.x;
			results[1].y	= m_Max.y;
			results[1].z	= m_Min.z;

			results[2].x	= m_Max.x;
			results[2].y	= m_Max.y;
			results[2].z	= m_Min.z;

			results[3].x	= m_Max.x;
			results[3].y	= m_Min.y;
			results[3].z	= m_Min.z;

			results[4]		= m_Max;

			results[5].x	= m_Min.x;
			results[5].y	= m_Max.y;
			results[5].z	= m_Max.z;

			results[6].x	= m_Min.x;
			results[6].y	= m_Min.y;
			results[6].z	= m_Max.z;

			results[7].x	= m_Max.x;
			results[7].y	= m_Min.y;
			results[7].z	= m_Max.z;

			return true;
		}
		else
		{
			return false;
		}
	}
};