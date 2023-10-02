#include "public.h"
layout(location = 0) in vec2 inUV;

layout(location = 0) out vec4 posTarget;
layout(location = 1) out vec4 extentTarget;

layout(binding = BINDING_TEXTURE0) uniform sampler2D posTex;
layout(binding = BINDING_TEXTURE1) uniform sampler2D extentTex;

layout(binding = BINDING_OBJECT)
uniform Object_DYN_UNIFORM
{
	int dimX;
	int dimY;
}object;

void main()
{
	ivec2 coord = ivec2(round(inUV * (vec2(object.dimX, object.dimY) - vec2(1.0))));
	posTarget = texelFetch(posTex, coord, 0);
	extentTarget = texelFetch(extentTex, coord, 0);
}