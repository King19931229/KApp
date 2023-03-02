#include "public.h"

layout(location = POSITION) in vec3 position;
layout(location = NORMAL) in vec3 normal;
layout(location = TEXCOORD0) in vec2 texcoord0;

layout(binding = BINDING_OBJECT)
uniform Object
{
	mat4 model;
	uint index;
}object;

void main()
{
	gl_Position = static_cascaded.light_view_proj[object.index] * object.model * vec4(position, 1.0);
}