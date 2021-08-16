#include "vertexinput.h"

layout(location = 0) out vec2 uv;

layout(location = 1) out vec4 outWorldPos;

layout(location = 2) out vec4 outViewPos;
layout(location = 3) out vec4 outViewNormal;
layout(location = 4) out vec4 outViewLightDir;

#if TANGENT_BINORMAL_INPUT
layout(location = 5) out vec4 outViewTangent;
layout(location = 6) out vec4 outViewBinormal;
#endif

#if !INSTANCE_INPUT
layout(binding = BINDING_OBJECT)
uniform Object
{
	mat4 model;
	mat4 prev_model;
}object;
#endif

void main()
{
	uv = texcoord0;

	mat4 worldMatrix = WORLD_MATRIX;
	vec4 worldNormal = worldMatrix * vec4(normal, 0.0);

#if TANGENT_BINORMAL_INPUT
	vec4 worldTangent = worldMatrix * vec4(tangent, 0.0);
	vec4 worldBinormal = worldMatrix * vec4(binormal, 0.0);
	outViewTangent = camera.view * worldTangent;
	outViewBinormal = camera.view * worldBinormal;
#endif

	outWorldPos = worldMatrix * vec4(position, 1.0);
	outViewPos = camera.view * outWorldPos;
	outViewNormal = camera.view * worldNormal;
	outViewLightDir = camera.view * global.sunLightDir;

	gl_Position = camera.proj * outViewPos;
}