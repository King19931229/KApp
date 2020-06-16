#include "vertexinput.h"

layout(location = 0) out vec2 uv;
layout(location = 1) out vec4 inWorldPos;
layout(location = 2) out vec4 inViewPos;

layout(binding = BINDING_OBJECT)
uniform Object
{
	mat4 model;
}object;

void main()
{
	uv = texcoord0;
	mat4 worldMatrix = WORLD_MATRIX;
	inWorldPos = WORLD_MATRIX * vec4(position, 1.0);
	inViewPos = camera.view * inWorldPos;
	gl_Position = camera.proj * inViewPos;
}