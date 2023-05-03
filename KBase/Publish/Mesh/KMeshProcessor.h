#pragma once
#include "KBase/Interface/IKAssetLoader.h"

class KMeshProcessor
{
public:
	struct Vertex
	{
		glm::vec3 pos;
		glm::vec2 uv;
		glm::vec3 tangent;
		glm::vec3 binormal;
	};

	bool CalcTangentBinormal(std::vector<Vertex>& vertices)
	{

	}
};