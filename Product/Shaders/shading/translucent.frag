#include "public.h"

layout(location = 0) in vec2 texCoord;
layout(location = 1) in vec3 worldPos;
layout(location = 2) in vec3 prevWorldPos;
layout(location = 3) in vec3 worldNormal;
#if TANGENT_BINORMAL_INPUT
layout(location = 4) in vec3 worldTangent;
layout(location = 5) in vec3 worldBinormal;
#endif

#ifdef GPU_SCENE
layout(location = 12) in flat uint darwIndex;
#endif

#define BINDING_STATIC_CSM0 BINDING_TEXTURE5
#define BINDING_STATIC_CSM1 BINDING_TEXTURE6
#define BINDING_STATIC_CSM2 BINDING_TEXTURE7
#define BINDING_STATIC_CSM3 BINDING_TEXTURE8

#include "shadow/cascaded/static_mask.h"

#define BINDING_DYNAMIC_CSM0 BINDING_TEXTURE9
#define BINDING_DYNAMIC_CSM1 BINDING_TEXTURE10
#define BINDING_DYNAMIC_CSM2 BINDING_TEXTURE11
#define BINDING_DYNAMIC_CSM3 BINDING_TEXTURE12

#include "shadow/cascaded/dynamic_mask.h"

layout(binding = BINDING_TEXTURE13) uniform samplerCube diffuseIrradiance;
layout(binding = BINDING_TEXTURE14) uniform samplerCube specularIrradiance;
layout(binding = BINDING_TEXTURE15) uniform sampler2D integrateBRDF;

#ifdef DEPTH_PEELING_TRANSRPANT_PASS
layout(binding = BINDING_TEXTURE16) uniform sampler2D prevPeelingDepth;
#endif

#ifdef DEPTH_PEELING_OUTPUT_TO_ABUFFER
layout(binding = BINDING_TEXTURE16, r32ui) uniform uimage2D LinkHeader;
layout(std430, binding = BINDING_TEXTURE17) buffer LinkNextBuffer { uint LinkNext[]; };
layout(std430, binding = BINDING_TEXTURE18) buffer LinkResultBuffer { vec4 LinkResult[]; };
layout(std430, binding = BINDING_TEXTURE19) buffer LinkDepthBuffer { float LinkDepth[]; };
#define maxABufferSize global.miscs[0]
#endif

layout(location = 0) out vec4 outColor;

/* Shader compiler will replace this into the texcode of the material */
#include "material_generate_code.h"

#include "pbr.h"

#include "shading_public.h"

layout(binding = BINDING_DEBUG)
uniform Debug_DYN_UNIFORM
{
	uint debugOption;
} debug;

void main()
{
	MaterialPixelParameters parameters = ComputeMaterialPixelParameters(
		  worldPos
		, prevWorldPos
		, worldNormal
		, texCoord
#if TANGENT_BINORMAL_INPUT
		, worldTangent
		, worldBinormal
#endif
		);

	vec2 motion = parameters.motion;
	vec3 albedo = parameters.baseColor;
	vec3 normal = parameters.normal;
	vec3 emissive = parameters.emissive;
	float metallic = parameters.metal;
	float roughness = parameters.roughness;
	float ao = parameters.ao;
	float opacity = parameters.opacity;

	vec4 cameraPos = camera.viewInv * vec4(0.0, 0.0, 0.0, 1.0);

	vec3 N = normalize(parameters.normal.xyz);
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

	vec2 envBRDF = texture(integrateBRDF, vec2(NdotV, roughness)).rg;
	vec3 specularIBL = prefilteredColor * (F * envBRDF.x + envBRDF.y);

	vec3 ambient = kD * diffuseIBL + specularIBL;

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
	vec3 direct = Lo;

	float shadow = min(CalcStaticCSM(worldPos.xyz), CalcDynamicCSM(worldPos.xyz)).r;
	direct *= shadow;

	ambient *= IBL_FACTOR;
	ambient *= ao;
	ambient *= shadow;

	vec3 final = ambient + direct + emissive;

	vec4 finalColor = vec4(final, opacity);

	finalColor += vec4(0.2, 0.5, 0.8, 0.0);
	finalColor.a = 0.3;

	uint debugOption = debug.debugOption;
	switch(debugOption)
	{
		case DRD_NONE:
			break;
		case DRD_ALBEDO:
			finalColor = vec4(albedo, 1.0);
			break;
		case DRD_NORMAL:
			finalColor = vec4(normal, 1.0);
			break;
		case DRD_METAL:
			finalColor = vec4(vec3(metallic), 1.0);
			break;
		case DRD_ROUGHNESS:
			finalColor = vec4(vec3(roughness), 1.0);
			break;
		case DRD_AO:
			finalColor = vec4(vec3(ao), 1.0);
			break;
		case DRD_EMISSIVE:
			finalColor = vec4(emissive, 1.0);
			break;
		case DRD_IBL_DIFFUSE:
			finalColor = vec4(diffuseIBL, 1.0);
			break;
		case DRD_IBL_SPECULAR:
			finalColor = vec4(specularIBL, 1.0);
			break;
		case DRD_IBL:
			finalColor = vec4(ambient, 1.0);
			break;
		case DRD_DIRECTLIGHT_DIFFUSE:
			finalColor = vec4(diffuseLight, 1.0);
			break;
		case DRD_DIRECTLIGHT_SPECULAR:
			finalColor = vec4(specularLight, 1.0);
			break;
		case DRD_DIRECTLIGHT:
			finalColor = vec4(Lo, 1.0);
			break;
		case DRD_NOV:
			finalColor = vec4(vec3(NdotV), 1.0);
			break;
		case DRD_NOL:
			finalColor = vec4(vec3(NdotL), 1.0);
			break;
		case DRD_VOH:
			finalColor = vec4(vec3(VdotH), 1.0);
			break;
		case DRD_NDF:
			finalColor = vec4(vec3(NDF), 1.0);
			break;
		case DRD_G:
			finalColor = vec4(vec3(G), 1.0);
			break;
		case DRD_F:
			finalColor = vec4(F, 1.0);
			break;
		case DRD_KS:
			finalColor = vec4(kS, 1.0);
			break;
		case DRD_KD:
			finalColor = vec4(kD, 1.0);
			break;
		case DRD_DIRECT:
			finalColor = vec4(direct, 1.0);
			break;
		case DRD_INDIRECT:
			finalColor = vec4(vec3(0), 1.0);
			break;
		case DRD_SCATTERING:
			finalColor = vec4(vec3(0), 1.0);
			break;
		case DRD_MOTION:
			finalColor = vec4(motion, 0, 0);
			break;
	}

#ifdef DEPTH_PEELING_OUTPUT_TO_ABUFFER
	ivec2 pixelCoord = ivec2(gl_FragCoord.xy);
	uint newHeaderIndex = 1 + atomicAdd(LinkNext[0], 1);
	if (newHeaderIndex >= maxABufferSize)
	{
		discard;
	}
	uint oldHeaderIndex = imageLoad(LinkHeader, pixelCoord).x;
	LinkNext[newHeaderIndex] = oldHeaderIndex;
	LinkResult[newHeaderIndex] = finalColor;
	LinkDepth[newHeaderIndex] = gl_FragCoord.z;
	imageStore(LinkHeader, pixelCoord, uvec4(newHeaderIndex));
	discard;
#else
	outColor = finalColor;
#endif
}