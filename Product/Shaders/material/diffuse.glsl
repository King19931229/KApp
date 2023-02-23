#include "public.h"

layout(binding = BINDING_MATERIAL0) uniform sampler2D diffuseSampler;

#if HAS_MATERIAL_TEXTURE1
layout(binding = BINDING_MATERIAL1) uniform sampler2D normalSampler;
#endif

#if HAS_MATERIAL_TEXTURE2
layout(binding = BINDING_MATERIAL2) uniform sampler2D specularGlosinessSampler;
#endif

#if HAS_MATERIAL_TEXTURE3
layout(binding = BINDING_MATERIAL3) uniform sampler2D emissiveSampler;
#endif

#if HAS_MATERIAL_TEXTURE4
layout(binding = BINDING_MATERIAL4) uniform sampler2D aoSampler;
#endif

layout(binding = BINDING_SHADING)
uniform Shading
{
	vec4 diffuseFactor;
	vec4 specularFactor;
	float metallicFactor;
	float roughnessFactor;
	float alphaMask;
	float alphaMaskCutoff;
} shading;

MaterialPixelParameters ComputeMaterialPixelParameters(
	  vec3 worldPos
	, vec3 prevWorldPos
	, vec3 worldNormal
	, vec2 texCoord
#if TANGENT_BINORMAL_INPUT
	, vec3 worldTangent
	, vec3 worldBinormal
#endif
	)
{
	MaterialPixelParameters parameters;

	parameters.position = worldPos;

#if (TANGENT_BINORMAL_INPUT && HAS_MATERIAL_TEXTURE1)
	vec4 normalmap = 2.0 * texture(normalSampler, texCoord) - vec4(1.0);
	parameters.normal = normalize(worldTangent * normalmap.r
					  + worldBinormal * normalmap.g
					  + worldNormal * normalmap.b);
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