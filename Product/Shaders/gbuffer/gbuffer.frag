layout(early_fragment_tests) in;

layout(location = 0) in vec4 worldNormal;
layout(location = 1) in vec4 worldPos;
layout(location = 2) in vec4 prevWorldPos;

layout(location = 0) out vec4 out0;
layout(location = 1) out vec4 out1;
layout(location = 2) out vec2 out2;

#include "public.h"

void main()
{
	out0 = vec4(normalize(worldNormal.rgb), worldNormal.a);
	out1 = worldPos;
	vec4 prev = camera.prevViewProj * prevWorldPos;
	vec4 curr = camera.viewProj * worldPos;
	vec2 prevUV = 0.5 * (prev.xy / prev.w + vec2(1, 1));
	vec2 currUV = 0.5 * (curr.xy / curr.w + vec2(1, 1));
	out2 = currUV - prevUV;
}