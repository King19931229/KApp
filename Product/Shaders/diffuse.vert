#include "vertexinput.h"

layout(location = 0) out vec2 uv;

layout(location = 1) out vec4 outWorldPos;

layout(location = 2) out vec4 outViewPos;
layout(location = 3) out vec4 outViewNormal;
layout(location = 4) out vec4 outViewLightDir;

#if !INSTANCE_INPUT
layout(binding = BINDING_OBJECT)
uniform Object
{
	mat4 model;
}object;
#endif

void main()
{
	uv = texcoord0;

	mat4 worldMatrix = WORLD_MATRIX;
	vec4 worldNormal = worldMatrix * vec4(normal, 0.0);

	outWorldPos = worldMatrix * vec4(position, 1.0);
	outViewPos = camera.view * outWorldPos;
	outViewNormal = camera.view * worldNormal;
	outViewLightDir = camera.view * global.sunLightDir;

	gl_Position = camera.proj * outViewPos;
}