#include "public.h"
layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 outColor;

layout(binding = BINDING_TEXTURE0) uniform sampler2D updateTex;

void main()
{
	outColor.rg = texture(updateTex, inUV).rg;
	outColor.ba = vec2(0);
}