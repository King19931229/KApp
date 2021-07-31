#include "public.h"

layout(location = POSITION) in vec3 position;
layout(location = NORMAL) in vec3 normal;
layout(location = TEXCOORD0) in vec2 texcoord0;

layout(binding = BINDING_OBJECT)
uniform Object
{
	mat4 model;
}object;

layout(location = 0) out vec4 encoded0;
layout(location = 1) out vec4 encoded1;

void main()
{
	encoded0.rgb = mat3(object.model) * normal;
	encoded1 = object.model * vec4(position, 1.0);
	gl_Position = camera.proj * camera.view * encoded1;
	encoded0.a = gl_Position.z / gl_Position.w;
	
}