#version 450
#extension GL_ARB_separate_shader_objects : enable
#include "public.glh"

layout(location = POSITION) in vec3 position;
layout(location = NORMAL) in vec3 normal;
layout(location = TEXCOORD0) in vec2 texcoord0;

//layout(location = DIFFUSE) in vec3 diffuse;
//layout(location = SPECULAR) in vec3 specular;

//layout(location = TANGENT) in vec3 tangent;
//layout(location = BINORMAL) in vec3 binormal;

layout(push_constant)
uniform PushConstant
{
	mat4 model;
}object;

layout(location = 0) out vec2 uv;
layout(location = 1) out vec4 inWorldPos;
layout(location = 2) out vec4 inViewPos;

void main()
{
	uv = texcoord0;
	inWorldPos = object.model * vec4(position, 1.0);
	inViewPos = camera.view * inWorldPos;
	gl_Position = camera.proj * inViewPos;
}