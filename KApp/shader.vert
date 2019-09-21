#version 450
#extension GL_ARB_separate_shader_objects : enable
#include "public.glh"

layout(location = POSITION) in vec3 position;
layout(location = NORMAL) in vec3 normal;
layout(location = TEXCOORD0) in vec3 uv;

layout(binding = TRANSFORM)
uniform UniformBufferObject
{
    mat4 model;
    mat4 view;
    mat4 proj;
}transform;

layout(location = 0) out vec3 fragColor;

vec3 colors[4] = vec3[](
    vec3(1.0, 0.0, 0.0),
    vec3(0.0, 1.0, 0.0),
    vec3(0.0, 0.0, 1.0),
	vec3(1.0, 0.0, 0.0)
);

void main()
{
	gl_Position = transform.proj * transform.view * transform.model * vec4(position.x, position.y, position.z, 1.0);
	//gl_Position = vec4(position.x, position.y, position.z, 1.0) * transform.model * transform.view * transform.proj;
	fragColor = colors[gl_VertexIndex];
}