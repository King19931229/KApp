#include "vertexinput.h"

#if !INSTANCE_INPUT
layout(binding = BINDING_OBJECT)
uniform Object_DYN_UNIFORM
{
	mat4 model;
	mat4 prev_model;
}object;
#endif

layout(location = 0) out vec4 inWorldPos;
layout(location = 1) out vec4 inViewPos;

void main()
{
	inWorldPos = worldMatrix * vec4(position, 1.0);
	inViewPos = camera.view * inWorldPos;
	gl_Position = camera.proj * inViewPos;
}