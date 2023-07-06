#pragma once
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "KBase/Publish/KStringParser.h"
#include <string>

namespace KMath
{
	inline float FloatPrecision(float value)
	{
		const int32_t exponentBits = 8;
		const int32_t mantissaBits = 23;
		const int32_t exponentShift = ((uint32_t)1 << (exponentBits - 1)) - 1;
		uint32_t valueAsInt = *(uint32_t*)(&value);
		valueAsInt &= (uint32_t)(~0) >> 1;
		float exponent = (float)pow(2.0f, (int32_t)(valueAsInt >> mantissaBits) - exponentShift);
		return (float)pow(2.0f, -mantissaBits - 1) * exponent;
	}

	inline double FloatPrecision(double value)
	{
		const int32_t exponentBits = 11;
		const int32_t mantissaBits = 52;
		const int32_t exponentShift = ((uint64_t)1 << (exponentBits - 1)) - 1;
		uint64_t valueAsInt = *(uint64_t*)(&value);
		valueAsInt &= (uint64_t)(~0) >> 1;
		double exponent = pow(2.0, (int32_t)(valueAsInt >> mantissaBits) - exponentShift);
		return pow(2.0, -mantissaBits - 1) * exponent;
	}

	inline float ScaleFactorToSameExponent(float original, float target)
	{
		uint32_t expOriginal = (*(uint32_t*)&original & ((uint32_t)~0 >> 1)) >> 23;
		uint32_t expTarget = (*(uint32_t*)&target & ((uint32_t)~0 >> 1)) >> 23;
		int32_t exp = 127 + (int32_t)expTarget - (int32_t)expOriginal;
		int32_t resAsInt = exp << 23;
		float factor = *(float*)&resAsInt;
		return factor;
	}

	inline double ScaleFactorToSameExponent(double original, double target)
	{
		uint64_t expOriginal = (*(uint64_t*)&original & ((uint64_t)~0 >> 1)) >> 52;
		uint64_t expTarget = (*(uint64_t*)&target & ((uint64_t)~0 >> 1)) >> 52;
		int64_t exp = 1023 + (int64_t)expTarget - (int64_t)expOriginal;
		int64_t resAsInt = exp << 52;
		double factor = *(double*)&resAsInt;
		return factor;
	}

	inline uint32_t ExponentCount(float value)
	{
		return 8;
	}

	inline uint32_t ExponentCount(double value)
	{
		return 11;
	}

	inline uint32_t MantissaCount(float value)
	{
		return 23;
	}

	inline uint32_t MantissaCount(double value)
	{
		return 52;
	}

	template <class T>
	inline T DivideAndRoundUp(T dividend, T divisor)
	{
		return (dividend + divisor - 1) / divisor;
	}

	template <class T>
	inline T DivideAndRoundDown(T dividend, T divisor)
	{
		return dividend / divisor;
	}

	template <class T>
	inline T DivideAndRoundNearest(T dividend, T divisor)
	{
		return (dividend >= 0)
			? (dividend + divisor / 2) / divisor
			: (dividend - divisor / 2 + 1) / divisor;
	}

	// 永远返回正数
	template<typename T>
	inline T Mod_Positive(T x, T y)
	{
		if (y < 0) y = -y;
		if (x < 0) x += (-x / y + 1) * y;
		return x % y;
	}

	template<typename T>
	inline glm::tvec3<T> ExtractPosition(const glm::tmat4x4<T>& transform)
	{
		glm::tvec4<T> transformPos = transform * glm::tvec4<T>(0, 0, 0, 1);
		return glm::tvec3<T>(transformPos.x / transformPos.w, transformPos.y / transformPos.w, transformPos.z / transformPos.w);
	}

	template<typename T>
	inline glm::tmat3x3<T> ExtractRotate(const glm::tmat4x4<T>& transform)
	{
		glm::tvec3<T> xAxis = transform * glm::tvec4<T>(1, 0, 0, 0);
		glm::tvec3<T> yAxis = transform * glm::tvec4<T>(0, 1, 0, 0);
		glm::tvec3<T> zAxis = transform * glm::tvec4<T>(0, 0, 1, 0);

		xAxis = glm::normalize(xAxis);
		yAxis = glm::normalize(yAxis);
		zAxis = glm::normalize(zAxis);

		glm::tmat3x3<T> rotate = glm::tmat3x3<T>(xAxis, yAxis, zAxis);

		// z轴不是x.cross(y) 是负缩放导致的
		if (glm::dot(glm::cross(xAxis, yAxis), zAxis) < 0)
		{
			rotate *= glm::tmat3x3<T>(-1);
		}
		
		return rotate;
	}

	template<typename T>
	inline glm::tvec3<T> ExtractScale(const glm::tmat4x4<T>& transform)
	{
		glm::tmat4x4<T> translate = glm::translate(glm::tmat4x4<T>(1), ExtractPosition(transform));
		glm::tmat4x4<T> scale = glm::inverse(translate * glm::tmat4x4<T>(ExtractRotate(transform))) * transform;
		return glm::tvec3<T>(scale[0][0], scale[1][1], scale[2][2]);
	}

	template<typename T>
	T SmallestPowerOf2GreaterEqualThan(T x)
	{
		T result = 1;
		while (result < x)
		{
			result <<= 1;
		}
		return result;
	}

	template<typename T>
	T BiggestPowerOf2LessEqualThan(T x)
	{
		return SmallestPowerOf2GreaterEqualThan(x >> 1);
	}

	inline uint32_t MortonCode2(uint32_t x)
	{
		x &= 0x0000ffff;
		x = (x ^ (x << 8)) & 0x00ff00ff;
		x = (x ^ (x << 4)) & 0x0f0f0f0f;
		x = (x ^ (x << 2)) & 0x33333333;
		x = (x ^ (x << 1)) & 0x55555555;
		return x;
	}

	inline uint64_t MortonCode2_64(uint64_t x)
	{
		x &= 0x00000000ffffffff;
		x = (x ^ (x << 16)) & 0x0000ffff0000ffff;
		x = (x ^ (x << 8)) & 0x00ff00ff00ff00ff;
		x = (x ^ (x << 4)) & 0x0f0f0f0f0f0f0f0f;
		x = (x ^ (x << 2)) & 0x3333333333333333;
		x = (x ^ (x << 1)) & 0x5555555555555555;
		return x;
	}

	inline uint32_t ReverseMortonCode2(uint32_t x)
	{
		x &= 0x55555555;
		x = (x ^ (x >> 1)) & 0x33333333;
		x = (x ^ (x >> 2)) & 0x0f0f0f0f;
		x = (x ^ (x >> 4)) & 0x00ff00ff;
		x = (x ^ (x >> 8)) & 0x0000ffff;
		return x;
	}

	inline uint64_t ReverseMortonCode2_64(uint64_t x)
	{
		x &= 0x5555555555555555;
		x = (x ^ (x >> 1)) & 0x3333333333333333;
		x = (x ^ (x >> 2)) & 0x0f0f0f0f0f0f0f0f;
		x = (x ^ (x >> 4)) & 0x00ff00ff00ff00ff;
		x = (x ^ (x >> 8)) & 0x0000ffff0000ffff;
		x = (x ^ (x >> 16)) & 0x00000000ffffffff;
		return x;
	}

	inline uint32_t MortonCode3(uint32_t x)
	{
		x &= 0x000003ff;
		x = (x ^ (x << 16)) & 0xff0000ff;
		x = (x ^ (x << 8)) & 0x0300f00f;
		x = (x ^ (x << 4)) & 0x030c30c3;
		x = (x ^ (x << 2)) & 0x09249249;
		return x;
	}

	inline uint32_t ReverseMortonCode3(uint32_t x)
	{
		x &= 0x09249249;
		x = (x ^ (x >> 2)) & 0x030c30c3;
		x = (x ^ (x >> 4)) & 0x0300f00f;
		x = (x ^ (x >> 8)) & 0xff0000ff;
		x = (x ^ (x >> 16)) & 0x000003ff;
		return x;
	}

	inline bool FromString(const std::string& text, glm::vec3& vec)
	{
		if (KStringParser::ParseToFLOAT(text.c_str(), &vec[0], 3))
		{
			return true;
		}
		return false;
	}

	inline bool FromString(const std::string& text, glm::vec4& vec)
	{
		if (KStringParser::ParseToFLOAT(text.c_str(), &vec[0], 4))
		{
			return true;
		}
		return false;
	}

	inline bool FromString(const std::string& text, glm::mat3& mat)
	{
		if (KStringParser::ParseToFLOAT(text.c_str(), &mat[0][0], 9))
		{
			return true;
		}
		return false;
	}

	inline bool FromString(const std::string& text, glm::mat4& mat)
	{
		if (KStringParser::ParseToFLOAT(text.c_str(), &mat[0][0], 16))
		{
			return true;
		}
		return false;
	}

	inline bool FromString(const std::string& text, glm::quat& quat)
	{
		if (KStringParser::ParseToFLOAT(text.c_str(), &quat[0], 4))
		{
			return true;
		}
		return false;
	}

	inline bool ToString(const glm::vec3& vec, std::string& text)
	{
		char szBuffer[256] = { 0 };
		if (KStringParser::ParseFromFLOAT(szBuffer, sizeof(szBuffer) - 1, &vec[0], 3))
		{
			text = szBuffer;
			return true;
		}
		return false;
	}

	inline bool ToString(const glm::vec4& vec, std::string& text)
	{
		char szBuffer[256] = { 0 };
		if (KStringParser::ParseFromFLOAT(szBuffer, sizeof(szBuffer) - 1, &vec[0], 4))
		{
			text = szBuffer;
			return true;
		}
		return false;
	}

	inline bool ToString(const glm::mat3& mat, std::string& text)
	{
		char szBuffer[256] = { 0 };
		if (KStringParser::ParseFromFLOAT(szBuffer, sizeof(szBuffer) - 1, &mat[0][0] , 9))
		{
			text = szBuffer;
			return true;
		}
		return false;
	}

	inline bool ToString(const glm::mat4& mat, std::string& text)
	{
		char szBuffer[256] = { 0 };
		if (KStringParser::ParseFromFLOAT(szBuffer, sizeof(szBuffer) - 1, &mat[0][0], 16))
		{
			text = szBuffer;
			return true;
		}
		return false;
	}

	inline bool ToString(const glm::quat& quat, std::string& text)
	{
		char szBuffer[256] = { 0 };
		if (KStringParser::ParseFromFLOAT(szBuffer, sizeof(szBuffer) - 1, &quat[0], 4))
		{
			text = szBuffer;
			return true;
		}
		return false;
	}
}