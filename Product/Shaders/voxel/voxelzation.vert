#include "public.h"

layout(location = POSITION) in vec3 position;
layout(location = NORMAL) in vec3 normal;
layout(location = TEXCOORD0) in vec2 texcoord0;

layout(binding = BINDING_OBJECT)
uniform Object
{
	mat4 model;
	mat4 prev_model;
}object;

layout(location = 0) out Vertex
{
	vec2 texcoord;
	vec3 normal;
} OUT;

void main()
{
	gl_Position = object.model * vec4(position, 1.0f);
	OUT.normal = (mat4(mat3(object.model)) * vec4(normal, 0.0f)).xyz;
	OUT.texcoord = texcoord0;
}