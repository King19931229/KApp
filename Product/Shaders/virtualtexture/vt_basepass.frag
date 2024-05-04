layout(location = 0) in vec2 texCoord;
layout(location = 1) in vec3 worldPos;
layout(location = 2) in vec3 prevWorldPos;
layout(location = 3) in vec3 worldNormal;
#if TANGENT_BINORMAL_INPUT
layout(location = 4) in vec3 worldTangent;
layout(location = 5) in vec3 worldBinormal;
#endif

layout(location = 0) out vec4 FeedbackRT;

/* Shader compiler will replace this into the texcode of the material */
#include "material_generate_code.h"

void main()
{
	MaterialPixelParameters parameters = ComputeMaterialPixelParameters(
		  worldPos
		, prevWorldPos
		, worldNormal
		, texCoord
#if TANGENT_BINORMAL_INPUT
		, worldTangent
		, worldBinormal
#endif
		);
}