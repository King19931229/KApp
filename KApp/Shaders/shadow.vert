#version 450
#extension GL_ARB_separate_shader_objects : enable
#include "public.glh"

layout(location = POSITION) in vec3 position;
layout(location = NORMAL) in vec3 normal;
layout(location = TEXCOORD0) in vec2 texcoord0;

layout(push_constant)
uniform PushConstant
{
	mat4 model;
}object;

void main()
{
	gl_Position = shadow.light_proj * shadow.light_view * object.model * vec4(position, 1.0);
}