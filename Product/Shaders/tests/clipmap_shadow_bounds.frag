#include "public.h"
#include "shadow/shadow_sample.h"

layout(binding = BINDING_TEXTURE0) uniform sampler2D shadowMap;
layout(location = 0) out vec4 outColor;

const vec3 samplePositions[8] = vec3[8]
(
	vec3(0.5, 0.5, 0.1),
	vec3(0.5, 0.5, 0.75),
	vec3(-0.25, 0.5, 0.75),
	vec3(1.25, 0.5, 0.75),
	vec3(0.5, -0.25, 0.75),
	vec3(0.5, 1.25, 0.75),
	vec3(0.5, 0.5, -0.25),
	vec3(0.5, 0.5, 1.25)
);

void main()
{
	vec3 position = samplePositions[int(gl_FragCoord.x)];
	outColor = vec4(HardShadow(shadowMap, position.xy, position.z),
		SoftShadow(shadowMap, vec2(0.01), 1.0, 10.0, position.xy, position.z, vec2(0.0), 5.0), 0.0, 1.0);
}
