#version 450
#extension GL_ARB_separate_shader_objects : enable
layout(early_fragment_tests) in;

layout(location = 0) in vec2 uv;
layout(location = 1) in vec4 inWorldPos;
layout(location = 2) in vec4 inViewPos;

layout(location = 0) out vec4 outColor;

#include "public.h"
#include "shadow.h"

layout(binding = BINDING_DIFFUSE) uniform sampler2D diffuseSampler;

void main()
{
	outColor = texture(diffuseSampler, uv);	
	outColor *= calcCSM(inViewPos.xyz);		
}