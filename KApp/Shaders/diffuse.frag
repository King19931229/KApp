#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 outColor;

#include "public.glh"

layout(binding = TEXTURE_SLOT0) uniform sampler2D texSampler;

void main()
{
	outColor = texture(texSampler, uv);
}