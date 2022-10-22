#include "public.h"

MaterialPixelParameters ComputeMaterialPixelParameters(
	  vec3 worldPos
	, vec3 prevWorldPos
	, vec3 worldNormal
	, vec2 texCoord
#if TANGENT_BINORMAL_INPUT
	, vec3 tangent
	, vec3 binormal
#endif
	)
{
	MaterialPixelParameters parameters;

	parameters.position = worldPos;

	parameters.normal = worldNormal;

	vec4 prev = camera.prevViewProj * vec4(prevWorldPos, 1.0);
	vec4 curr = camera.viewProj * vec4(worldPos, 1.0);
	vec2 prevUV = 0.5 * (prev.xy / prev.w + vec2(1, 1));
	vec2 currUV = 0.5 * (curr.xy / curr.w + vec2(1, 1));
	parameters.motion = currUV - prevUV;

	parameters.baseColor = vec3(1, 0, 0);

	parameters.specularColor = vec3(0, 0, 0);

	return parameters;
}