layout(early_fragment_tests) in;

layout(location = 0) out vec4 outColor;

#include "public.h"
#include "shadow.h"

layout(location = 0) in vec4 inWorldPos;
layout(location = 1) in vec4 inViewPos;

layout(binding = BINDING_FRAGMENT_SHADING)
uniform Parameter
{
 	vec4 color;
}parameter;

void main()
{
	outColor = parameter.color;
	outColor *= calcCSM(inViewPos.xyz, inWorldPos.xyz);
}