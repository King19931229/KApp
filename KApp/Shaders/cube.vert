#version 450
#extension GL_ARB_separate_shader_objects : enable
#include "public.glh"

layout(location = POSITION) in vec3 position;
layout(location = NORMAL) in vec3 normal;
layout(location = TEXCOORD0) in vec2 texcoord0;

layout(push_constant)uniform PushConstant
{
	mat4 model;
}object;

layout(binding = CAMERA)
uniform CameraInfo
{
    mat4 view;
    mat4 proj;
	mat4 viewInv;
}camera;

layout(location = 0) out vec3 uvw;

void main()
{
	uvw = normalize(position);
	gl_Position = camera.proj * mat4(mat3(camera.view)) * object.model * vec4(position, 1.0);
	gl_Position.z = gl_Position.w;
}