#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 uv;
layout(location = 1) in vec3 worldPos;
layout(location = 2) in vec3 worldNormal;
layout(location = 3) in vec3 worldEye;

layout(binding = 1) uniform sampler2D texSampler;

layout(location = 0) out vec4 outColor;

#include "public.glh"

void main()
{
	outColor = texture(texSampler, uv);
}