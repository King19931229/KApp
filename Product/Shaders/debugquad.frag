#include "public.h"

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 outColor;

layout(binding = BINDING_TEXTURE0) uniform sampler2D texSampler;

void main()
{
	outColor = vec4(texture(texSampler, uv).rrr, 1.0);
}