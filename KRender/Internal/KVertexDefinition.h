#pragma once
#include "Interface/IKRenderConfig.h"
#include "glm/glm.hpp"
#include <assert.h>

#pragma pack(push, 1)

struct KVertexPositionNormalUV
{
	glm::vec3 poistion;
	glm::vec3 normal;
	glm::vec3 uv;
};

struct KVertexUV2
{
	glm::vec3 uv2;
};

struct KVertexDiffuseSpecular
{
	glm::vec3 diffuse;
	glm::vec3 specular;
};

struct KVertexTangentBinormal
{
	glm::vec3 tangent;
	glm::vec3 binormal;
};

struct KVertexBlendWeightsBlendIndices
{
	glm::vec4 blendWeights;
	glm::ivec4 blendIndices;
};

#pragma pack(pop)

namespace KVertexDefinition
{
	int SemanticOffsetInElement(VertexSemantic semantic, VertexElement element);
}