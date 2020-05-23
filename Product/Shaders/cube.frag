#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 uvw;
layout(location = 0) out vec4 outColor;

#include "public.glh"

layout(binding = TEXTURE_SLOT0) uniform samplerCube samplerEnvMap;

void main()
{
	outColor = texture(samplerEnvMap, CUBEMAP_UVW(uvw));
}