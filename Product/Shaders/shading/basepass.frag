layout(early_fragment_tests) in;

layout(location = 0) in vec2 texCoord;
layout(location = 1) in vec3 worldPos;
layout(location = 2) in vec3 prevWorldPos;
layout(location = 3) in vec3 worldNormal;
#if TANGENT_BINORMAL_INPUT
layout(location = 4) in vec3 tangent;
layout(location = 5) in vec3 binormal;
#endif

layout(location = 0) out vec4 RT0;
layout(location = 1) out vec4 RT1;
layout(location = 2) out vec4 RT2;
layout(location = 3) out vec4 RT3;

/* Shader compiler will replace this into the texcode of the material */
#include "material_generate_code.h"

void EncodeGBuffer(vec3 pos, vec3 normal, vec2 motion, vec3 baseColor, vec3 specularColor)
{
	vec4 clipPos = camera.viewProj * vec4(pos, 1.0);
	float depth = clipPos.z / clipPos.w;
	RT0.xyz = normal;
	RT0.w = depth;
	RT1.xy = 0.5 * (motion + vec2(1.0));
	RT2.xyz = baseColor;
	RT3.xyz = specularColor;
}

void main()
{
	MaterialPixelParameters parameters = ComputeMaterialPixelParameters(
		  worldPos
		, prevWorldPos
		, worldNormal
		, texCoord
#if TANGENT_BINORMAL_INPUT
		, tangent
		, binormal
#endif
		);

	EncodeGBuffer(
		parameters.position,
		parameters.normal,
		parameters.motion,
		parameters.baseColor,
		parameters.specularColor
		);
}