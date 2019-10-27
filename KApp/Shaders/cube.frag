#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 uvw;

layout(binding = 1) uniform samplerCube samplerEnvMap;

layout(location = 0) out vec4 outColor;

#include "public.glh"

void main()
{
	outColor = texture(samplerEnvMap, CUBEMAP_UVW(uvw));
}