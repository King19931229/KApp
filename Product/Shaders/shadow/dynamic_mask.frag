#include "public.h"
#define cascaded_shadow dynamic_cascaded 
#include "shadow.h"

layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 outColor;

layout(binding = SHADOW_BINDING_GBUFFER_POSITION) uniform sampler2D position;

void main()
{
	vec4 worldPos = vec4(texture(position, inUV).xyz, 1.0);
	vec4 viewPos = camera.view * worldPos;
	outColor = CalcCSM(viewPos.xyz, worldPos.xyz);
}