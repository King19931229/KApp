#include "public.h"
layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 outColor;

layout(binding = BINDING_TEXTURE0) uniform sampler2D uploadTex;

void main()
{
	outColor = vec4(inUV,0,1);
	outColor = texture(uploadTex, inUV);
}