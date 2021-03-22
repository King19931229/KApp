#include "public.h"
layout (location = SCREENQAUD_POS) in vec2 inPos;
layout (location = 0) out vec2 outUV;
layout (location = 1) out vec3 outPos;

out gl_PerVertex 
{
	vec4 gl_Position;
};

layout(binding = BINDING_OBJECT)
uniform Object
{
	vec4 up;
	vec4 right;
	vec4 center;
}object;

void main() 
{
	outUV = (inPos + vec2(1.0, 1.0)) * 0.5;
	outPos = (object.center + object.right * inPos.x + object.up *inPos.y).xyz; 
	gl_Position = vec4(inPos, 0.0, 1.0);
}