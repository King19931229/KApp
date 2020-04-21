#pragma once
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtc/matrix_transform.hpp"

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
}