#include "public.h"

layout(location = POSITION) in vec3 position;

layout(binding = BINDING_OBJECT)
uniform Object_DYN_UNIFORM
{
	mat4 model;
	mat4 prev_model;
	vec4 color;
}object;

layout(location = 0) out vec4 color;

void main()
{
	gl_Position = camera.proj * camera.view * object.model * vec4(position, 1.0);
	color = object.color;
}