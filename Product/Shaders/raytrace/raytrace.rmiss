#extension GL_EXT_ray_tracing : require
#extension GL_GOOGLE_include_directive : enable
#include "raycommon.h"

layout(location = 0) rayPayloadInEXT HitPayload prd;

void main()
{
	// View-independent background gradient to simulate a basic sky background
	const vec3 gradientStart = vec3(0.5, 0.6, 1.0);
	const vec3 gradientEnd = vec3(1.0);
	vec3 unitDir = normalize(gl_WorldRayDirectionEXT);
	float t = 0.5 * (unitDir.y + 1.0);
	prd.hitValue = (1.0-t) * gradientStart + t * gradientEnd;
	prd.attenuation = vec3(1.0) - prd.sum;
	prd.done = 1;
}