#include "public.h"

layout(binding = BINDING_TEXTURE0) uniform sampler2D diffuseSampler;
layout(binding = BINDING_TEXTURE1) uniform sampler2D normalSampler;

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

#if (TANGENT_BINORMAL_INPUT && HAS_MATERIAL_TEXTURE1)
	vec4 normalmap = 2.0 * texture(normalSampler, uv) - vec4(1.0);
	parameters.normal = normalize(tangent) * normalmap.r +
	normalize(binormal) * normalmap.g +
	normalize(inViewNormal) * normalmap.b;
#else
	parameters.normal = worldNormal;
#endif

	vec4 prev = camera.prevViewProj * vec4(prevWorldPos, 1.0);
	vec4 curr = camera.viewProj * vec4(worldPos, 1.0);
	vec2 prevUV = 0.5 * (prev.xy / prev.w + vec2(1.0));
	vec2 currUV = 0.5 * (curr.xy / curr.w + vec2(1.0));
	parameters.motion = currUV - prevUV;

#if HAS_MATERIAL_TEXTURE0
	parameters.baseColor = texture(diffuseSampler, texCoord).rgb;
#else
	parameters.baseColor = vec3(1, 0, 0);
#endif

	parameters.specularColor = vec3(0, 0, 0);

	parameters.roughness = 0.0;
	parameters.metal = 0.0;

	return parameters;
}