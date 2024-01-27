layout(location = 0) in vec2 texCoord;
layout(location = 1) in vec3 worldPos;
layout(location = 2) in vec3 prevWorldPos;
layout(location = 3) in vec3 worldNormal;
#if TANGENT_BINORMAL_INPUT
layout(location = 4) in vec3 worldTangent;
layout(location = 5) in vec3 worldBinormal;
#endif

#if VERTEX_COLOR_INPUT0
layout(location = 6) in vec3 color0;
#endif
#if VERTEX_COLOR_INPUT1
layout(location = 7) in vec3 color1;
#endif
#if VERTEX_COLOR_INPUT2
layout(location = 8) in vec3 color2;
#endif
#if VERTEX_COLOR_INPUT3
layout(location = 9) in vec3 color3;
#endif
#if VERTEX_COLOR_INPUT4
layout(location = 10) in vec3 color4;
#endif
#if VERTEX_COLOR_INPUT5
layout(location = 11) in vec3 color5;
#endif

layout(location = 0) out vec4 RT0;
layout(location = 1) out vec4 RT1;
layout(location = 2) out vec4 RT2;
layout(location = 3) out vec4 RT3;
layout(location = 4) out vec4 RT4;

/* Shader compiler will replace this into the texcode of the material */
#include "material_generate_code.h"

void EncodeGBuffer(vec3 pos, vec3 normal, vec2 motion, vec3 baseColor, vec3 emissive, float metal, float roughness, float ao)
{
	vec4 viewPos = camera.view * vec4(pos, 1.0);
	float near = camera.proj[3][2] / camera.proj[2][2];
	float far = -camera.proj[3][2] / (camera.proj[2][3] - camera.proj[2][2]);
	float depth = (-viewPos.z - near) / (far - near);
	RT0.xyz = normalize(normal);
	RT0.w = depth;
	RT1.xy = vec2(motion.x, motion.y);
	RT2.xyz = baseColor;
	RT2.w = ao;
	RT3.x = metal;
	RT3.y = roughness;
	RT4.xyz = emissive;
}

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

	EncodeGBuffer(
		parameters.position,
		parameters.normal,
		parameters.motion,
		parameters.baseColor,
		parameters.emissive,
		parameters.metal,
		parameters.roughness,
		parameters.ao
		);
}