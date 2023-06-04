#pragma once
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "KBase/Publish/KStringParser.h"
#include <string>

namespace KMath
{
	template<typename T>
	inline T FloatPrecision(T value);

	template<>
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

	template<>
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

	template<typename T>
	inline uint32_t MantissaCount(T value);

	template<>
	inline uint32_t MantissaCount(float value)
	{
		return 23;
	}

	template<>
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

	// 永远返回正数
	template<typename T>
	inline T Mod(T x, T y)
	{
		if (y < 0) y = -y;
		if (x < 0) x += (-x / y + 1) * y;
		return x % y;
	}

	inline glm::vec3 ExtractPosition(const glm::mat4& transform)
	{
		glm::vec4 transformPos = transform * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
		return glm::vec3(transformPos.x / transformPos.w, transformPos.y / transformPos.w, transformPos.z / transformPos.w);
	}

	inline glm::mat3 ExtractRotate(const glm::mat4& transform)
	{
		glm::vec3 xAxis = transform * glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
		glm::vec3 yAxis = transform * glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
		glm::vec3 zAxis = transform * glm::vec4(0.0f, 0.0f, 1.0f, 0.0f);

		xAxis = glm::normalize(xAxis);
		yAxis = glm::normalize(yAxis);
		zAxis = glm::normalize(zAxis);

		glm::mat3 rotate = glm::mat3(xAxis, yAxis, zAxis);

		// z轴不是x.cross(y) 是负缩放导致的
		if (glm::dot(glm::cross(xAxis, yAxis), zAxis) < 0.0f)
		{
			rotate *= glm::mat3(-1.0f);
		}
		
		return rotate;
	}

	inline glm::vec3 ExtractScale(const glm::mat4& transform)
	{
		glm::mat4 translate = glm::translate(glm::mat4(1.0f), ExtractPosition(transform));
		glm::mat4 scale = glm::inverse(translate * glm::mat4(ExtractRotate(transform))) * transform;
		return glm::vec3(scale[0][0], scale[1][1], scale[2][2]);
	}

	template<typename T>
	T SmallestPowerOf2GreaterThan(T x)
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
		return SmallestPowerOf2GreaterThan(x >> 1);
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