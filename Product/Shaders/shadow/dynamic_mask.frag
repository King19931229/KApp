#include "public.h"
#define cascaded_shadow dynamic_cascaded 
#include "shadow.h"

layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 outColor;

layout(binding = BINDING_TEXTURE0) uniform sampler2D gbuffer0;

void main()
{
	float depth = texture(gbuffer0, inUV).w;
	vec3 ndc = vec3(2.0 * inUV - vec2(1.0), depth);
	vec4 worldPos = camera.viewInv * camera.projInv * vec4(ndc, 1.0);
	worldPos /= worldPos.w;
	vec4 viewPos = camera.view * worldPos;
	outColor = CalcCSM(viewPos.xyz, worldPos.xyz);
}