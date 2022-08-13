#include "public.h"
layout (location = SCREENQAUD_POS) in vec2 inPos;
layout (location = 0) out vec2 outUV;

void main() 
{
	vec2 pos = (inPos + vec2(1.0, 1.0)) * 0.5;
	outUV = pos;
	gl_Position = vec4(inPos, 0.0, 1.0);
}