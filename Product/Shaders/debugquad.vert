#version 450
#include "public.glh"

layout (location = SCREENQAUD_POS) in vec2 inPos;
layout (location = 0) out vec2 outUV;

out gl_PerVertex 
{
	vec4 gl_Position;
};

layout(push_constant)
uniform PushConstant
{
	mat4 matrix;
}clip;

void main() 
{
	outUV = (inPos + vec2(1.0, 1.0)) * 0.5;
	gl_Position = clip.matrix * vec4(inPos , 0.0, 1.0);
}