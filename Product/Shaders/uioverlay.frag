#version 450

#include "public.glh"

layout (location = 0) in vec2 inUV;
layout (location = 1) in vec4 inColor;

layout (location = 0) out vec4 outColor;

layout (binding = BINDING_TEXTURE0) uniform sampler2D fontSampler;

void main() 
{
	outColor = inColor * texture(fontSampler, inUV);
}