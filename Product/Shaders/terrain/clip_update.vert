#include "public.h"
layout (location = SCREENQAUD_POS) in vec2 inPos;
layout (location = 0) out vec2 outUV;

layout(binding = BINDING_OBJECT)
uniform Object
{
	// sx sy ex ey
	uvec4 rect;
	uvec2 size;
}object;

void main() 
{
	vec2 pos = (inPos + vec2(1.0, 1.0)) * 0.5;
	outUV = outUV;
	// scale
	pos.x *= float(object.rect[2] - object.rect[0] + 1) / float(object.size[0]);
	pos.y *= float(object.rect[3] - object.rect[1] + 1) / float(object.size[1]);
	// translate
	pos.x += float(object.rect[0]) / float(object.size[0]);
	pos.y += float(object.rect[1]) / float(object.size[1]);
	// normalize
	pos = pos * 2.0 - vec2(1.0);

	gl_Position = vec4(pos, 0.0, 1.0);
}