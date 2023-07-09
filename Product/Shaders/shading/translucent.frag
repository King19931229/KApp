#include "public.h"

layout(location = 0) in vec2 texCoord;
layout(location = 1) in vec3 worldPos;
layout(location = 2) in vec3 prevWorldPos;
layout(location = 3) in vec3 worldNormal;
#if TANGENT_BINORMAL_INPUT
layout(location = 4) in vec3 worldTangent;
layout(location = 5) in vec3 worldBinormal;
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

layout(location = 0) out vec4 outColor;

/* Shader compiler will replace this into the texcode of the material */
#include "material_generate_code.h"

#include "pbr.h"

#include "shading_public.h"

layout(binding = BINDING_DEBUG)
uniform Debug
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

	outColor = vec4(final, opacity); 

	uint debugOption = debug.debugOption;
	switch(debugOption)
	{
		case DRD_NONE:
			break;
		case DRD_ALBEDO:
			outColor = vec4(albedo, 1.0);
			break;
		case DRD_NORMAL:
			outColor = vec4(normal, 1.0);
			break;
		case DRD_METAL:
			outColor = vec4(vec3(metallic), 1.0);
			break;
		case DRD_ROUGHNESS:
			outColor = vec4(vec3(roughness), 1.0);
			break;
		case DRD_AO:
			outColor = vec4(vec3(ao), 1.0);
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
			outColor = vec4(vec3(NdotV), 1.0);
			break;
		case DRD_NOL:
			outColor = vec4(vec3(NdotL), 1.0);
			break;
		case DRD_VOH:
			outColor = vec4(vec3(VdotH), 1.0);
			break;
		case DRD_NDF:
			outColor = vec4(vec3(NDF), 1.0);
			break;
		case DRD_G:
			outColor = vec4(vec3(G), 1.0);
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
			outColor = vec4(vec3(0), 1.0);
			break;
		case DRD_SCATTERING:
			outColor = vec4(vec3(0), 1.0);
			break;
	}
}