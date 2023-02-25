#include "public.h"

layout(location = 0) in vec2 screenCoord;

layout(binding = BINDING_TEXTURE0) uniform sampler2D gbuffer0;
layout(binding = BINDING_TEXTURE1) uniform sampler2D gbuffer1;
layout(binding = BINDING_TEXTURE2) uniform sampler2D gbuffer2;
layout(binding = BINDING_TEXTURE3) uniform sampler2D gbuffer3;

layout(binding = BINDING_TEXTURE4) uniform sampler2D shadowMask;
layout(binding = BINDING_TEXTURE5) uniform sampler2D giMask;
layout(binding = BINDING_TEXTURE6) uniform sampler2D aoMask;
layout(binding = BINDING_TEXTURE7) uniform sampler2D scatteringMask;

layout(binding = BINDING_TEXTURE8) uniform samplerCube diffuseIrradiance;
layout(binding = BINDING_TEXTURE9) uniform samplerCube specularIrradiance;
layout(binding = BINDING_TEXTURE10) uniform sampler2D integrateBRDF;

layout(location = 0) out vec4 outColor;

#include "gbuffer.h"
#include "util.h"
#include "pbr.h"

void DecodeGBuffer(in vec2 uv, out vec3 worldPos, out vec3 worldNormal, out vec2 motion, out vec3 baseColor, out float metal, out float roughness)
{
	vec4 gbuffer0Data = texture(gbuffer0, uv);
	vec4 gbuffer1Data = texture(gbuffer1, uv);
	vec4 gbuffer2Data = texture(gbuffer2, uv);
	vec4 gbuffer3Data = texture(gbuffer3, uv);

	worldPos = DecodePosition(gbuffer0Data, uv);
	worldNormal = DecodeNormal(gbuffer0Data);
	motion = DecodeMotion(gbuffer1Data);
	baseColor = DecodeBaseColor(gbuffer2Data);
	metal = DecodeMetal(gbuffer3Data);
	roughness = DecodeRoughness(gbuffer3Data);
}

void main()
{
	vec3 worldPos;
	vec3 worldNormal;
	vec2 motion;
	vec3 albedo;
	float metallic;
	float roughness;
	float ao = 1.0;

	DecodeGBuffer(screenCoord, worldPos, worldNormal, motion, albedo, metallic, roughness);

	// ao combine
	ao *= texture(aoMask, screenCoord).r;

	vec4 cameraPos = camera.viewInv * vec4(0.0, 0.0, 0.0, 1.0);

	vec3 N = normalize(worldNormal.xyz);
	vec3 V = normalize(cameraPos.xyz - worldPos.xyz);
	vec3 R = reflect(-V, N);
	vec3 L = -global.sunLightDirAndMaxPBRLod.xyz;

	float NdotL = max(dot(N, L), 0.0);
	float NdotV = max(dot(N, V), 0.0);

	vec3 F0 = vec3(0.04);
	F0 = mix(F0, albedo, metallic);

	vec3 F = FresnelSchlickRoughness(NdotV, F0, roughness);

	vec3 kS = F;
	vec3 kD = vec3(1.0) - kS;
	kD *= 1.0 - metallic;

	// IBL
	float MAX_REFLECTION_LOD = global.sunLightDirAndMaxPBRLod.w;
	vec3 irradiance = TextureCubeFixed(diffuseIrradiance, CUBEMAP_UVW(N)).rgb;
	vec3 diffuse    = irradiance * albedo;

	vec3 prefilteredColor = textureLod(specularIrradiance, CUBEMAP_UVW(R), roughness * MAX_REFLECTION_LOD).rgb;   
	vec2 envBRDF  = texture(integrateBRDF, vec2(NdotV, roughness)).rg;
	vec3 specular = prefilteredColor * (F * envBRDF.x + envBRDF.y);

	vec3 ambient = (kD * diffuse + specular) * ao;

	// DirectLight
	// cook-torrance brdf
	vec3 H = normalize(V + L);
	float VdotH = max(dot(V, H), 0.0);

	float NDF = DistributionGGX(N, H, roughness);
	float G = GeometrySmithGGXJoint(N, V, L, roughness); // G = GeometrySmithGGX(N, V, 0.0), roughness);
	F = FresnelSchlick(VdotH, F0); // F = FresnelSchlick(NdotV, F0);

	vec3 numerator = NDF * G * F;
	float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
	vec3 lightdiffuse = kD * albedo / PI;
	vec3 lightSpecular = numerator / max(denominator, 0.001);

	vec3 Lo = vec3(0.0);
	vec3 radiance = vec3(1.0);
	Lo += (lightdiffuse + lightSpecular) * radiance * NdotL;
	vec3 direct = ambient + Lo;

	// calculate per-light radiance
	// ...

	// shadow combine
	direct *= texture(shadowMask, screenCoord).r;

	vec3 indirect = texture(giMask, screenCoord).rgb;
	vec3 final = direct + ao * indirect;

	vec4 scattering = texture(scatteringMask, screenCoord);
	final = final * scattering.a + scattering.rgb;

	outColor = vec4(final, 1.0);
}