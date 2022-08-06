#include "public.h"
layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 outColor;

layout(binding = BINDING_TEXTURE0) uniform sampler2D updateTex;

void main()
{
	outColor = texture(updateTex, inUV);
	// outColor = vec4(1.0, 1.0, 0, 0);
}