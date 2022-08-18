#include "public.h"
layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 outColor;

layout(binding = BINDING_TEXTURE0) uniform sampler2D updateTex;

layout(binding = BINDING_OBJECT)
uniform Object
{
	vec4 area;
}object;

in vec4 gl_FragCoord;

void main()
{
	ivec2 coord = ivec2(round(inUV * (object.area.xy - vec2(1.0))));
	outColor.rg = texelFetch(updateTex, coord, 0).rg;
	outColor.ba = vec2(0);
}