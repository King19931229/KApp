#include "vertexinput.h"

#if !INSTANCE_INPUT
layout(binding = BINDING_OBJECT)
uniform Object
{
	mat4 model;
}object;
#endif

layout(location = 0) out vec4 inWorldPos;
layout(location = 1) out vec4 inViewPos;

void main()
{
	mat4 worldMatrix = WORLD_MATRIX;
	inWorldPos = WORLD_MATRIX * vec4(position, 1.0);
	inViewPos = camera.view * inWorldPos;
	gl_Position = camera.proj * inViewPos;
}