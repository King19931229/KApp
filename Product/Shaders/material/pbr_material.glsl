#include "public.h"

#if HAS_MATERIAL_TEXTURE0
layout(binding = BINDING_TEXTURE0) uniform sampler2D diffuseSampler;
#endif

#if HAS_MATERIAL_TEXTURE1
layout(binding = BINDING_TEXTURE1) uniform sampler2D normalSampler;
#endif

#if HAS_MATERIAL_TEXTURE2
#if PBR_MATERIAL_SPECULAR_GLOSINESS
layout(binding = BINDING_MATERIAL2) uniform sampler2D specularGlosinessSampler;
#else
layout(binding = BINDING_TEXTURE2) uniform sampler2D metalRoughnessSampler;
#endif
#endif

#if HAS_MATERIAL_TEXTURE3
layout(binding = BINDING_TEXTURE3) uniform sampler2D emissiveSampler;
#endif

#if HAS_MATERIAL_TEXTURE4
layout(binding = BINDING_TEXTURE4) uniform sampler2D aoSampler;
#endif

#define MANUAL_SRGB 1
#define SRGB_FAST_APPROXIMATION 1

const float MIN_ROUGHNESS = 0.03;
const float EPSILON = 1e-6;

float ConvertMetallic(vec3 diffuse, vec3 specular, float maxSpecular)
{
	float perceivedDiffuse = sqrt(0.299 * diffuse.r * diffuse.r + 0.587 * diffuse.g * diffuse.g + 0.114 * diffuse.b * diffuse.b);
	float perceivedSpecular = sqrt(0.299 * specular.r * specular.r + 0.587 * specular.g * specular.g + 0.114 * specular.b * specular.b);
	if (perceivedSpecular < MIN_ROUGHNESS)
	{
		return 0.0;
	}
	float a = MIN_ROUGHNESS;
	float b = perceivedDiffuse * (1.0 - maxSpecular) / (1.0 - MIN_ROUGHNESS) + perceivedSpecular - 2.0 * MIN_ROUGHNESS;
	float c = MIN_ROUGHNESS - perceivedSpecular;
	float D = max(b * b - 4.0 * a * c, 0.0);
	return clamp((-b + sqrt(D)) / (2.0 * a), 0.0, 1.0);
}

vec4 SRGBtoLINEAR(vec4 srgbIn)
{
#ifdef MANUAL_SRGB
	#ifdef SRGB_FAST_APPROXIMATION
	vec3 linOut = pow(srgbIn.xyz,vec3(2.2));
	#else //SRGB_FAST_APPROXIMATION
	vec3 bLess = step(vec3(0.04045),srgbIn.xyz);
	vec3 linOut = mix( srgbIn.xyz/vec3(12.92), pow((srgbIn.xyz+vec3(0.055))/vec3(1.055),vec3(2.4)), bLess );
	#endif //SRGB_FAST_APPROXIMATION
	return vec4(linOut,srgbIn.w);
#else //MANUAL_SRGB
	return srgbIn;
#endif //MANUAL_SRGB
}

layout(binding = BINDING_SHADING)
uniform Shading
{
	vec4 baseColorFactor;
#if PBR_MATERIAL_SPECULAR_GLOSINESS
	vec4 diffuseFactor;
	vec4 specularFactor;
#endif
	vec4 emissiveFactor;
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

#if HAS_MATERIAL_TEXTURE0
	vec4 diffuse = SRGBtoLINEAR(texture(diffuseSampler, texCoord));
	parameters.baseColor = diffuse.rgb * shading.baseColorFactor.rgb;
	parameters.opacity = diffuse.a * shading.baseColorFactor.a;
#else
	parameters.baseColor = shading.baseColorFactor.rgb;
	parameters.opacity = 1.0;
#endif

	if (shading.alphaMask > 0 && parameters.opacity <= shading.alphaMaskCutoff)
	{
		discard;
	}

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
	parameters.motion = prevUV - currUV;

#if PBR_MATERIAL_METAL_ROUGHNESS
	parameters.roughness = shading.roughnessFactor;
	parameters.metal = shading.metallicFactor;
	#if HAS_MATERIAL_TEXTURE2
	// Roughness is stored in the 'g' channel, metallic is stored in the 'b' channel.
	// This layout intentionally reserves the 'r' channel for (optional) occlusion map data
	vec4 metalRoughness = texture(metalRoughnessSampler, texCoord);
	parameters.metal *= metalRoughness.b;
	parameters.roughness *= metalRoughness.g;
	#else
	parameters.metal = clamp(parameters.metal, 0.0, 1.0);
	parameters.roughness = clamp(parameters.roughness, MIN_ROUGHNESS, 1.0);
	#endif
#endif

#if PBR_MATERIAL_SPECULAR_GLOSINESS
	vec3 specular;
	#if HAS_MATERIAL_TEXTURE2
	vec4 specularGlosiness = texture(specularGlosinessSampler, texCoord);
	parameters.roughness = 1.0 - specularGlosiness.a;
	specular = SRGBtoLINEAR(specularGlosiness).rgb;
	#else
	parameters.roughness = 0;
	specular = vec3(0);
	#endif

	vec4 diffuse;
	#if HAS_MATERIAL_TEXTURE0
	diffuse = SRGBtoLINEAR(texture(diffuseSampler, texCoord));
	#else
	diffuse = vec4(0);
	#endif

	float maxSpecular = max(max(specular.r, specular.g), specular.b);

	// Convert metallic value from specular glossiness inputs
	float metallic = ConvertMetallic(diffuse.rgb, specular, maxSpecular);

	vec3 baseColorDiffusePart = diffuse.rgb * ((1.0 - maxSpecular) / (1 - MIN_ROUGHNESS) / max(1 - metallic, EPSILON)) * shading.diffuseFactor.rgb;
	vec3 baseColorSpecularPart = specular - (vec3(MIN_ROUGHNESS) * (1 - metallic) * (1 / max(metallic, EPSILON))) * shading.specularFactor.rgb;
	
	parameters.baseColor = vec4(mix(baseColorDiffusePart, baseColorSpecularPart, metallic * metallic), diffuse.a).rgb;
	parameters.metal = metallic;
#endif

#if HAS_MATERIAL_TEXTURE3
	parameters.emissive = SRGBtoLINEAR(texture(emissiveSampler, texCoord)).rgb;
#else
	parameters.emissive = vec3(0);
#endif

#if HAS_MATERIAL_TEXTURE4
	parameters.ao = texture(aoSampler, texCoord).r;
#else
	parameters.ao = 1.0;
#endif

	return parameters;
}