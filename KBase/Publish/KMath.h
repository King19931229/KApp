#pragma once
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "KBase/Publish/KStringParser.h"
#include <string>

namespace KMath
{
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