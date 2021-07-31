layout(early_fragment_tests) in;

layout(location = 0) in vec4 encoded0;
layout(location = 1) in vec4 encoded1;

layout(location = 0) out vec4 out0;
layout(location = 1) out vec4 out1;

#include "public.h"

void main()
{
	out0 = encoded0;
	out1 = encoded1;
}