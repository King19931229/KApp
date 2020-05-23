#version 450
#extension GL_ARB_separate_shader_objects : enable
#include "public.glh"

layout(location = 0) in vec2 uv;
layout(binding = 0) uniform sampler2D texSampler0;
layout(binding = 1) uniform sampler2D texSampler1;
layout(location = 0) out vec4 outColor;

void main()
{
	outColor = mix(texture(texSampler0, uv), texture(texSampler1, uv), 0.5);
}