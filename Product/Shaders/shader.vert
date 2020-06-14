#include "public.h"

layout(location = POSITION) in vec3 position;
layout(location = NORMAL) in vec3 normal;
layout(location = TEXCOORD0) in vec2 texcoord0;

layout(binding = BINDING_OBJECT)
uniform Object
{
	mat4 model;
}object;

layout(location = 0) out vec2 uv;
layout(location = 1) out vec3 worldPos;
layout(location = 2) out vec3 worldNormal;
layout(location = 3) out vec3 worldEye;

void main()
{
	uv = texcoord0;

	worldPos = (object.model * vec4(position, 1.0)).xyz;
	worldNormal = mat3(object.model) * normal;

	worldEye = (camera.viewInv * vec4(vec3(0.0), 1.0)).xyz;

	gl_Position = camera.proj * camera.view * vec4(worldPos, 1.0);
}