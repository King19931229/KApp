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

void EncodeGBuffer(vec3 pos, vec3 normal, vec2 motion, vec3 baseColor, vec3 specularColor, float roughness, float metal)
{
	vec4 viewPos = camera.view * vec4(pos, 1.0);
	float near = camera.proj[3][2] / camera.proj[2][2];
	float far = -camera.proj[3][2] / (camera.proj[2][3] - camera.proj[2][2]);
	float depth = (-viewPos.z - near) / (far - near);
	RT0.xyz = normalize(normal);
	RT0.w = depth;
	RT1.xy = vec2(motion.x, motion.y);
	RT1.zw = vec2(0.0);
	RT2.xyz = baseColor;
	RT2.w = roughness;
	RT3.xyz = specularColor;
	RT3.w = metal;
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
		parameters.specularColor,
		parameters.roughness,
		parameters.metal
		);
}