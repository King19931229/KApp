#include "public.h"
layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 outColor;

layout(binding = BINDING_TEXTURE0) uniform sampler2D uploadTex;

void main()
{
	vec2 uv = vec2(inUV.x, 1 - inUV.y);
	outColor = vec4(uv,0,1);
	outColor = texture(uploadTex, uv);
}