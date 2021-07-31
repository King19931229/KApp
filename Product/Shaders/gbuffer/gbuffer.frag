layout(early_fragment_tests) in;

layout(location = 0) in vec3 worldNormal;
layout(location = 0) out vec4 outColor;

#include "public.h"

void main()
{
	outColor = vec4(worldNormal, 1.0);
}