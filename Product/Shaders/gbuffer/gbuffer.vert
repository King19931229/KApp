#include "public.h"

layout(location = POSITION) in vec3 position;
layout(location = NORMAL) in vec3 normal;
layout(location = TEXCOORD0) in vec2 texcoord0;

layout(binding = BINDING_OBJECT)
uniform Object
{
	mat4 model;
}object;

layout(location = 0) out vec3 worldNormal;

void main()
{
	worldNormal = mat3(object.model) * normal;
	gl_Position = camera.proj * camera.view * object.model * vec4(position, 1.0);
}