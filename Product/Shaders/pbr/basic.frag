layout(early_fragment_tests) in;

layout(location = 0) in vec2 uv;
layout(location = 1) in vec4 worldPos;
layout(location = 2) in vec4 worldNormal;
layout(location = 3) in vec4 cameraPos;

layout(location = 0) out vec4 outColor;

#include "public.h"
#include "shadow/shadow.h"

layout(binding = BINDING_FRAGMENT_SHADING)
uniform Shading
{
	vec4 lightPos;
	vec4 lightColor;
	vec4 albedo;
	// metallic,roughness,ao,mipmap
	vec4 attr;
}shading;

layout(binding = BINDING_DIFFUSE_IRRADIANCE) uniform samplerCube diffuseIrradiance;
layout(binding = BINDING_SPECULAR_IRRADIANCE) uniform samplerCube specularIrradiance;
layout(binding = BINDING_INTEGRATE_BRDF) uniform sampler2D integrateBRDF;

#define TEXTURE_BASE_PBR 0

#if TEXTURE_BASE_PBR
layout(binding = BINDING_MATERIAL0) uniform sampler2D baseColorSampler;
layout(binding = BINDING_MATERIAL1) uniform sampler2D metalicSampler;
layout(binding = BINDING_MATERIAL2) uniform sampler2D normalSampler;
layout(binding = BINDING_MATERIAL3) uniform sampler2D roughnessSampler;
#endif

void main()
{
	vec3 N = normalize(worldNormal.xyz);
	vec3 V = normalize(cameraPos.xyz - worldPos.xyz);
	vec3 R = reflect(-V, N);
#if TEXTURE_BASE_PBR
	float metallic = texture(metalicSampler, uv).r; // shading.attr[0];
	float roughness = texture(roughnessSampler, uv).r; // shading.attr[1];
	vec3 albedo = texture(baseColorSampler, uv).rgb; //shading.albedo.xyz;
#else
	float metallic = shading.attr[0];
	float roughness = shading.attr[1];
	vec3 albedo = shading.albedo.xyz;
#endif
	float ao = shading.attr[2];
	float MAX_REFLECTION_LOD = shading.attr[3];
	vec3 F0 = vec3(0.04);
	F0 = mix(F0, albedo, metallic);

	vec3 F = FresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);

	vec3 kS = F;
	vec3 kD = vec3(1.0) - kS;
	kD *= 1.0 - metallic;

	// IBL
	vec3 irradiance = TextureCubeFixed(diffuseIrradiance, CUBEMAP_UVW(N)).rgb;
	vec3 diffuse    = irradiance * albedo;

	vec3 prefilteredColor = textureLod(specularIrradiance, CUBEMAP_UVW(R), roughness * MAX_REFLECTION_LOD).rgb;   
	vec2 envBRDF  = texture(integrateBRDF, vec2(min(max(dot(N, V), 0.0), 1.0), roughness)).rg;
	vec3 specular = prefilteredColor * (F * envBRDF.x + envBRDF.y);
  
	vec3 ambient = (kD * diffuse + specular) * ao;

	// DirectLight
	/*
	vec3 numerator = NDF * G * F;
	float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
	vec3 lightSpecular = numerator / max(denominator, 0.001);

	// calculate per-light radiance
	vec3 Lo = vec3(0.0);

	vec3 L = normalize(shading.lightPos.xyz - worldPos.xyz);
	vec3 H = normalize(V + L);

	// cook-torrance brdf

	float NDF = DistributionGGX(N, H, roughness);
	float G = GeometrySmith(N, V, L, roughness); // G = GeometrySchlickGGX(max(dot(N, V), 0.0), roughness);
	vec3 F = FresnelSchlick(max(dot(H, V), 0.0), F0); // F = FresnelSchlick(max(dot(N, V), 0.0), F0);

	float distance = length(shading.lightPos.xyz - worldPos.xyz);
	float attenuation = 1.0;// / (distance * distance);
	vec3 radiance = shading.lightColor.xyz * attenuation;

	Lo += (kD * albedo / PI + lightSpecular) * radiance * max(dot(N, L), 0.0)); 
	vec3 color = ambient + Lo;
	*/
	outColor = vec4(ambient, 1);
}