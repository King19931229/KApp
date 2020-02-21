#version 450
#extension GL_ARB_separate_shader_objects : enable
layout(early_fragment_tests) in;

layout(location = 0) out vec4 outColor;

#include "public.glh"

void main()
{
	outColor = vec4(0.0, 0.0, 1.0, 1.0);
}