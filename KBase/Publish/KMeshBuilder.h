#pragma once
#include "KBase/Publish/KConfig.h"
#include "glm/glm.hpp"

namespace KMeshBuilder
{
	struct Vertex
	{
		glm::vec3 pos;
		glm::vec2 uv;
		glm::vec3 tangent;
		glm::vec3 binormal;
	};
}