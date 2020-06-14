layout(location = 0) in vec3 uvw;
layout(location = 0) out vec4 outColor;

#include "public.h"

layout(binding = BINDING_TEXTURE0) uniform samplerCube samplerEnvMap;

void main()
{
	outColor = texture(samplerEnvMap, CUBEMAP_UVW(uvw));
}