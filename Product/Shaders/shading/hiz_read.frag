#include "public.h"
#include "common.h"
layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 outColor;

layout(binding = BINDING_TEXTURE0) uniform sampler2D depthSampler;

void main()
{
	float depth = texture(depthSampler, inUV).r;
	outColor = vec4(depth);
}