#include "vertexinput.h"

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
	mat4 worldMatrix = WORLD_MATRIX;
	vec4 worldPos = worldMatrix * vec4(position, 1.0);
	gl_Position = camera.viewProj * worldPos;
}