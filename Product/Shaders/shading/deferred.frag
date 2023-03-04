#include "public.h"

#include "shading_public.h"

layout(location = 0) in vec2 screenCoord;

layout(binding = BINDING_TEXTURE0) uniform sampler2D gbuffer0;
layout(binding = BINDING_TEXTURE1) uniform sampler2D gbuffer1;
layout(binding = BINDING_TEXTURE2) uniform sampler2D gbuffer2;
layout(binding = BINDING_TEXTURE3) uniform sampler2D gbuffer3;
layout(binding = BINDING_TEXTURE4) uniform sampler2D gbuffer4;

layout(binding = BINDING_TEXTURE5) uniform sampler2D shadowMask;
layout(binding = BINDING_TEXTURE6) uniform sampler2D giMask;
layout(binding = BINDING_TEXTURE7) uniform sampler2D aoMask;
layout(binding = BINDING_TEXTURE8) uniform sampler2D scatteringMask;
layout(binding = BINDING_TEXTURE9) uniform sampler2D ssrMask;

layout(binding = BINDING_TEXTURE10) uniform samplerCube diffuseIrradiance;
layout(binding = BINDING_TEXTURE11) uniform samplerCube specularIrradiance;
layout(binding = BINDING_TEXTURE12) uniform sampler2D integrateBRDF;

layout(location = 0) out vec4 outColor;

#include "gbuffer.h"
#include "util.h"
#include "pbr.h"

void DecodeGBuffer(in vec2 uv, out vec3 worldPos, out vec3 worldNormal, out vec2 motion, out vec3 baseColor, out vec3 emissive, out float metal, out float roughness, out float ao)
{
	GBufferEncodeData encode;
	encode.gbuffer0 = texture(gbuffer0, uv);
	encode.gbuffer1 = texture(gbuffer1, uv);
	encode.gbuffer2 = texture(gbuffer2, uv);
	encode.gbuffer3 = texture(gbuffer3, uv);
	encode.gbuffer4 = texture(gbuffer4, uv);
	encode.uv = uv;

	GBufferDecodeData decode = DecodeGBufferData(encode);

	worldPos = decode.worldPos;
	worldNormal = decode.worldNormal;
	motion = decode.motion;
	baseColor = decode.baseColor;
	emissive = decode.emissive;
	metal = decode.metal;
	roughness = decode.roughness;
	ao = decode.ao;
}

layout(binding = BINDING_DEBUG)
uniform Debug
{
	uint debugOption;
} debug;

void main()
{
	vec3 worldPos;
	vec3 worldNormal;
	vec2 motion;
	vec3 albedo;
	vec3 emissive;
	float metallic;
	float roughness;
	float ao;

	DecodeGBuffer(screenCoord, worldPos, worldNormal, motion, albedo, emissive, metallic, roughness, ao);

	// ao combine
	ao *= texture(aoMask, screenCoord).r;

	vec4 cameraPos = camera.viewInv * vec4(0.0, 0.0, 0.0, 1.0);

	vec3 N = normalize(worldNormal.xyz);
	vec3 V = normalize(cameraPos.xyz - worldPos.xyz);
	vec3 R = normalize(reflect(-V, N));
	vec3 L = normalize(-global.sunLightDirAndMaxPBRLod.xyz);

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
	vec3 diffuseIBL = irradiance * albedo;

	vec3 prefilteredColor = textureLod(specularIrradiance, CUBEMAP_UVW(R), roughness * MAX_REFLECTION_LOD).rgb;   
	vec4 ssr = texture(ssrMask, screenCoord);
	prefilteredColor = mix(prefilteredColor, ssr.rgb, ssr.a);

	vec2 envBRDF = texture(integrateBRDF, vec2(NdotV, roughness)).rg;
	vec3 specularIBL = prefilteredColor * (F * envBRDF.x + envBRDF.y);

	vec3 ambient = (kD * diffuseIBL + specularIBL) * ao;

	// DirectLight
	// cook-torrance brdf
	vec3 H = normalize(V + L);
	float VdotH = max(dot(V, H), 0.0);

	float NDF = DistributionGGX(N, H, roughness);
	float G = GeometrySmithGGXJoint(N, V, L, roughness); // G = GeometrySmithGGX(N, V, 0.0), roughness);
	F = FresnelSchlick(VdotH, F0); // F = FresnelSchlick(NdotV, F0);

	vec3 numerator = NDF * G * F;
	float denominator = 4.0 * NdotV * NdotL;
	vec3 diffuseLight = kD * albedo / PI;
	vec3 specularLight = numerator / max(denominator, 0.001);

	vec3 Lo = vec3(0.0);
	vec3 radiance = vec3(1.0);
	Lo += (diffuseLight + specularLight) * radiance * NdotL;
	vec3 direct = ambient + Lo;

	// calculate per-light radiance
	// ...

	// shadow combine
	direct *= texture(shadowMask, screenCoord).r;

	vec3 indirect = texture(giMask, screenCoord).rgb;
	vec3 final = direct + emissive + ao * indirect;

	vec4 scattering = texture(scatteringMask, screenCoord);
	final = final * scattering.a + scattering.rgb;

	outColor = vec4(final, 1.0);

	uint debugOption = debug.debugOption;
	switch(debugOption)
	{
		case DRD_NONE:
			break;
		case DRD_ALBEDO:
			outColor = vec4(albedo, 1.0);
			break;
		case DRD_METAL:
			outColor = vec4(metallic);
			break;
		case DRD_ROUGHNESS:
			outColor = vec4(roughness);
			break;
		case DRD_AO:
			outColor = vec4(ao);
			break;
		case DRD_EMISSIVE:
			outColor = vec4(emissive, 1.0);
			break;
		case DRD_IBL_DIFFUSE:
			outColor = vec4(diffuseIBL, 1.0);
			break;
		case DRD_IBL_SPECULAR:
			outColor = vec4(specularIBL, 1.0);
			break;
		case DRD_IBL:
			outColor = vec4(ambient, 1.0);
			break;
		case DRD_DIRECTLIGHT_DIFFUSE:
			outColor = vec4(diffuseLight, 1.0);
			break;
		case DRD_DIRECTLIGHT_SPECULAR:
			outColor = vec4(specularLight, 1.0);
			break;
		case DRD_DIRECTLIGHT:
			outColor = vec4(Lo, 1.0);
			break;
		case DRD_NOV:
			outColor = vec4(NdotV);
			break;
		case DRD_NOL:
			outColor = vec4(NdotL);
			break;
		case DRD_VOH:
			outColor = vec4(VdotH);
			break;
		case DRD_NDF:
			outColor = vec4(NDF);
			break;
		case DRD_G:
			outColor = vec4(G);
			break;
		case DRD_F:
			outColor = vec4(F, 1.0);
			break;
		case DRD_KS:
			outColor = vec4(kS, 1.0);
			break;
		case DRD_KD:
			outColor = vec4(kD, 1.0);
			break;
		case DRD_DIRECT:
			outColor = vec4(direct, 1.0);
			break;
		case DRD_INDIRECT:
			outColor = vec4(indirect, 1.0);
			break;
		case DRD_SCATTERING:
			outColor = scattering;
			break;
	}
}