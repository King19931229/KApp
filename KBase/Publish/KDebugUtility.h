#pragma once
#include "glm/glm.hpp"

enum DebugPrimitive
{
	DEBUG_PRIMITIVE_LINE,
	DEBUG_PRIMITIVE_TRIANGLE
};

struct KMeshUtilityInfo
{
	std::vector<glm::vec3> positions;
	std::vector<uint16_t> indices;
	DebugPrimitive primtive;
};