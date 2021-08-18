layout(early_fragment_tests) in;

layout(location = 0) in vec4 worldNormal;
layout(location = 1) in vec4 worldPos;
layout(location = 2) in vec2 vel;

layout(location = 0) out vec4 out0;
layout(location = 1) out vec4 out1;
layout(location = 2) out vec2 out2;

#include "public.h"

void main()
{
	out0 = worldNormal;
	out1 = worldPos;
	out2 = vel;
}