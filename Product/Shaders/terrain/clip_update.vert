#include "public.h"
layout (location = SCREENQAUD_POS) in vec2 inPos;
layout (location = 0) out vec2 outUV;

layout(binding = BINDING_OBJECT)
uniform Object
{
	vec4 area;
}object;

void main() 
{
	vec2 pos = (inPos + vec2(1.0, 1.0)) * 0.5;
	outUV = pos;
	// scale and translate
	pos.xy = pos.xy * object.area.xy + object.area.zw;
	// normalize
	pos = pos * 2.0 - vec2(1.0);

	gl_Position = vec4(pos, 0.0, 1.0);
}