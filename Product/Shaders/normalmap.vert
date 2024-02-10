#include "vertexinput.h"

layout(location = 0) out vec2 uv;

layout(location = 1) out vec4 outWorldPos;

layout(location = 2) out vec4 outViewPos;

layout(location = 3) out vec4 outTangentLightDir;
layout(location = 4) out vec4 outTangentViewDir;
layout(location = 5) out vec4 outTangentPos;

#if !INSTANCE_INPUT
layout(binding = BINDING_OBJECT)
uniform Object_DYN_UNIFORM
{
	mat4 model;
	mat4 prev_model;
}object;
#endif

void main()
{
	uv = texcoord0;

#if TANGENT_BINORMAL_INPUT
	vec3 T = normalize(mat3(worldMatrix) * tangent);
	vec3 B = normalize(mat3(worldMatrix) * binormal);
	vec3 N = normalize(mat3(worldMatrix) * normal);
	mat3 TBN = transpose(mat3(T, B, N));
#else
	mat3 TBN = mat3(1.0);
#endif

	vec4 cameraPos = camera.viewInv * vec4(0.0, 0.0, 0.0, 1.0);

	outWorldPos = worldMatrix * vec4(position, 1.0);
	outViewPos = camera.view * outWorldPos;
	outTangentPos = vec4(TBN * outWorldPos.xyz, 1.0);
	outTangentViewDir = normalize(vec4(TBN * cameraPos.xyz, 1.0) - outTangentPos);
	outTangentLightDir = vec4(TBN * global.sunLightDirAndMaxPBRLod.xyz, 0.0);

	gl_Position = camera.proj * outViewPos;
}