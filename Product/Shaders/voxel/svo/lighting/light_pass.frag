#include "public.h"
#include "voxel/svo/voxel_common.h"
#include "shading/decode.h"

layout(location = 0) out vec4 fragColor;

layout(location = 0) in vec2 texCoord;

layout(binding = VOXEL_BINDING_GBUFFER_RT0) uniform sampler2D gbuffer0;
layout(binding = VOXEL_BINDING_GBUFFER_RT1) uniform sampler2D gbuffer1;
layout(binding = VOXEL_BINDING_GBUFFER_RT2) uniform sampler2D gbuffer2;
layout(binding = VOXEL_BINDING_GBUFFER_RT3) uniform sampler2D gbuffer3;

#ifndef USE_OCTREE
#define USE_OCTREE 0
#endif

#if USE_OCTREE
layout(binding = VOXEL_BINDING_OCTREE) buffer uuOctree { uint uOctree[]; };
layout(binding = VOXEL_BINDING_OCTREE_DATA) buffer uuOctreeData { uvec4 uOctreeData[]; };
layout(binding = VOXEL_BINDING_OCTREE_MIPMAP_DATA) buffer uuOctreeMipmapData { uint uOctreeMipmapData[][6]; };
#include "voxel/svo/octree/octree_common.h"
#include "voxel/svo/octree/octree_util.h"
#else
layout(binding = VOXEL_BINDING_NORMAL) uniform sampler3D voxelVisibility;
layout(binding = VOXEL_BINDING_RADIANCE) uniform sampler3D voxelTex;
#endif

layout(binding = VOXEL_BINDING_TEXMIPMAP_IN) uniform sampler3D voxelTexMipmap[6];

vec4 cameraPosition = camera.viewInv * vec4(0.0, 0.0, 0.0, 1.0);

const vec3 diffuseConeDirections[] =
{
	vec3(0.0f, 1.0f, 0.0f),
	vec3(0.0f, 0.5f, 0.866025f),
	vec3(0.823639f, 0.5f, 0.267617f),
	vec3(0.509037f, 0.5f, -0.7006629f),
	vec3(-0.50937f, 0.5f, -0.7006629f),
	vec3(-0.823639f, 0.5f, 0.267617f)
};

const float diffuseConeWeights[] =
{
	PI / 4.0f,
	3.0f * PI / 20.0f,
	3.0f * PI / 20.0f,
	3.0f * PI / 20.0f,
	3.0f * PI / 20.0f,
	3.0f * PI / 20.0f,
};

vec3 WorldToVoxel(vec3 position)
{
	vec3 voxelPos = position - voxel.minpoint_scale.xyz;
	return voxelPos * voxel.minpoint_scale.w;
}

vec4 AnistropicSample(vec3 coord, vec3 weight, uvec3 face, float lod)
{
	// anisotropic volumes level
	float anisoLevel = max(lod - 1.0f, 0.0f);
	// directional sample
// #if USE_OCTREE
// 	vec4 anisoSample = weight.x * SampleOctreeMipmap(volumeDimension, coord, anisoLevel, face.x)
// 					 + weight.y * SampleOctreeMipmap(volumeDimension, coord, anisoLevel, face.y)
// 					 + weight.z * SampleOctreeMipmap(volumeDimension, coord, anisoLevel, face.z);
// #else
	vec4 anisoSample = weight.x * textureLod(voxelTexMipmap[face.x], coord, anisoLevel)
					 + weight.y * textureLod(voxelTexMipmap[face.y], coord, anisoLevel)
					 + weight.z * textureLod(voxelTexMipmap[face.z], coord, anisoLevel);
// #endif

	// linearly interpolate on base level
// 	if (lod < 1.0f)
// 	{
// #if USE_OCTREE
// #else
// 		vec4 baseColor = texture(voxelTex, coord);
// 		anisoSample = mix(baseColor, anisoSample, clamp(lod, 0.0f, 1.0f));
// #endif
// 	}

	return anisoSample;
}

bool IntersectRayWithWorldAABB(vec3 ro, vec3 rd, out float enter, out float leave)
{
	vec3 tempMin = (voxel.minpoint_scale.xyz - ro) / rd; 
	vec3 tempMax = (voxel.maxpoint_scale.xyz - ro) / rd;

	vec3 v3Max = max (tempMax, tempMin);
	vec3 v3Min = min (tempMax, tempMin);

	leave = min (v3Max.x, min (v3Max.y, v3Max.z));
	enter = max (max (v3Min.x, 0.0), max (v3Min.y, v3Min.z));    

	return leave > enter;
}

vec4 TraceCone(vec3 position, vec3 normal, vec3 direction, float aperture, bool traceOcclusion)
{
	uvec3 visibleFace;
	visibleFace.x = (direction.x < 0.0) ? 0 : 1;
	visibleFace.y = (direction.y < 0.0) ? 2 : 3;
	visibleFace.z = (direction.z < 0.0) ? 4 : 5;
	traceOcclusion = traceOcclusion && aoAlpha < 1.0f;
	// world space grid voxel size
	// cone tracing in anistropic mipmap so that is 2.0 being divided not 1.0
	float voxelWorldSize = 2.0 / (voxel.minpoint_scale.w * volumeDimension);
	// weight per axis for aniso sampling
	vec3 weight = direction * direction;
	// move further to avoid self collision
	float dst = voxelWorldSize;
	vec3 startPosition = position + normal * dst;
	// final results
	vec4 coneSample = vec4(0.0f);
	float occlusion = 0.0f;
	float maxDistance = maxTracingDistanceGlobal * (1.0f / voxel.minpoint_scale.w);
	float falloff = 0.5f * aoFalloff * voxel.minpoint_scale.w;
	// out of boundaries check
	float enter = 0.0; float leave = 0.0;

	if(!IntersectRayWithWorldAABB(position, direction, enter, leave))
	{
		coneSample.a = 1.0f;
	}

	while(coneSample.a < 1.0f && dst <= maxDistance)
	{
		vec3 conePosition = startPosition + direction * dst;
		// cone expansion and respective mip level based on diameter
		float diameter = 2.0f * aperture * dst;
		float mipLevel = log2(diameter / voxelWorldSize);
		// convert position to texture coord
		vec3 coord = WorldToVoxel(conePosition);
		// get directional sample from anisotropic representation
		vec4 anisoSample = AnistropicSample(coord, weight, visibleFace, mipLevel);
		// front to back composition
		coneSample += (1.0f - coneSample.a) * anisoSample;
		// ambient occlusion
		if(traceOcclusion && occlusion < 1.0)
		{
			occlusion += ((1.0f - occlusion) * anisoSample.a) / (1.0f + falloff * diameter);
		}
		// move further into volume
		dst += diameter * samplingFactor;
	}

	return vec4(coneSample.rgb, occlusion);
}

float TraceShadowCone(vec3 position, vec3 direction, float aperture, float maxTracingDistance) 
{
	bool hardShadows = false;

	if(coneShadowTolerance == 1.0f) { hardShadows = true; }

	// directional dominat axis
	uvec3 visibleFace;
	visibleFace.x = (direction.x < 0.0) ? 0 : 1;
	visibleFace.y = (direction.y < 0.0) ? 2 : 3;
	visibleFace.z = (direction.z < 0.0) ? 4 : 5;
	// world space grid size
	float voxelWorldSize = 2.0 / (voxel.minpoint_scale.w * volumeDimension);
	// weight per axis for aniso sampling
	vec3 weight = direction * direction;
	// move further to avoid self collision
	float dst = voxelWorldSize;
	vec3 startPosition = position + direction * dst;
	// control vars
	float mipMaxLevel = log2(volumeDimension) - 1.0f;
	// final results
	float visibility = 0.0f;
	float k = exp2(7.0f * coneShadowTolerance);
	// cone will only trace the needed distance
	float maxDistance = maxTracingDistance;
	// out of boundaries check
	float enter = 0.0; float leave = 0.0;

	if(!IntersectRayWithWorldAABB(position, direction, enter, leave))
	{
		visibility = 1.0f;
	}

	while(visibility < 1.0f && dst <= maxDistance)
	{
		vec3 conePosition = startPosition + direction * dst;
		float diameter = 2.0f * aperture * dst;
		float mipLevel = log2(diameter / voxelWorldSize);
		// convert position to texture coord
		vec3 coord = WorldToVoxel(conePosition);
		// get directional sample from anisotropic representation
		vec4 anisoSample = AnistropicSample(coord, weight, visibleFace, mipLevel);

		// hard shadows exit as soon cone hits something
		if(hardShadows && anisoSample.a > EPSILON) { return 0.0f; }  
		// accumulate
		visibility += (1.0f - visibility) * anisoSample.a * k;
		// move further into volume
		dst += diameter * samplingFactor;
	}

	return 1.0f - visibility;
}

float Visibility(vec3 position)
{
	return 1.0;
}

vec3 Ambient(Light light, vec3 albedo)
{
	return max(albedo * light.ambient, 0.0f);
}

vec3 BRDF(Light light, vec3 N, vec3 X, vec3 ka, vec4 ks)
{
	// common variables
	vec3 L = light.direction;
	vec3 V = normalize(cameraPosition.xyz / cameraPosition.w - X);
	vec3 H = normalize(V + L);
	// compute dot procuts
	float dotNL = max(dot(N, L), 0.0f);
	float dotNH = max(dot(N, H), 0.0f);
	float dotLH = max(dot(L, H), 0.0f);
	// decode specular power
	float spec = exp2(11.0f * ks.a + 1.0f);
	// emulate fresnel effect
	vec3 fresnel = ks.rgb + (vec3(1.0f) - ks.rgb) * pow(1.0f - dotLH, 5.0f);
	// specular factor
	float blinnPhong = pow(dotNH, spec);
	// energy conservation, aprox normalization factor
	blinnPhong *= spec * 0.0397f + 0.3183f;
	// specular term
	vec3 specular = ks.rgb * light.specular * blinnPhong * fresnel;
	// diffuse term
	vec3 diffuse = ka.rgb * light.diffuse;
	// return composition
	return (diffuse + specular) * dotNL;
}

vec3 CalculateDirectional(Light light, vec3 normal, vec3 position, vec3 albedo, vec4 specular)
{
	float visibility = 1.0f;

	if(light.shadowingMethod == 1)
	{
		visibility = Visibility(position);
	}
	else if(light.shadowingMethod == 2)
	{
		visibility = max(0.0f, TraceShadowCone(position, light.direction, coneShadowAperture, 1.0f / voxel.minpoint_scale.w));
	}
	else if(light.shadowingMethod == 3)
	{
		vec3 voxelPos = WorldToVoxel(position);
#if USE_OCTREE
		visibility = max(0.0f, SampleOctreeVisibility(volumeDimension, voxelPos));
#else
		visibility = max(0.0f, texture(voxelVisibility, voxelPos).a);
#endif
	}

	if(visibility <= 0.0f) return vec3(0.0f);  

	return BRDF(light, normal, position, albedo, specular) * visibility;
}

vec3 CalculatePoint(Light light, vec3 normal, vec3 position, vec3 albedo, vec4 specular)
{
	light.direction = light.position - position;
	float d = length(light.direction);
	light.direction = normalize(light.direction);
	float falloff = 1.0f / (light.attenuation.constant + light.attenuation.linear * d
					+ light.attenuation.quadratic * d * d + 1.0f);

	if(falloff <= 0.0f) return vec3(0.0f);

	float visibility = 1.0f;

	if(light.shadowingMethod == 2)
	{
		visibility = max(0.0f, TraceShadowCone(position, light.direction, coneShadowAperture, d));
	}
	else if(light.shadowingMethod == 3)
	{
		vec3 voxelPos = WorldToVoxel(position);
#if USE_OCTREE
		visibility = max(0.0f, SampleOctreeVisibility(volumeDimension, voxelPos));
#else
		visibility = max(0.0f, texture(voxelVisibility, voxelPos).a);
#endif
	} 

	if(visibility <= 0.0f) return vec3(0.0f);  

	return BRDF(light, normal, position, albedo, specular) * falloff * visibility;
}

vec3 CalculateSpot(Light light, vec3 normal, vec3 position, vec3 albedo, vec4 specular)
{
	vec3 spotDirection = light.direction;
	light.direction = normalize(light.position - position);
	float cosAngle = dot(-light.direction, spotDirection);

	// outside the cone
	if(cosAngle < light.angleOuterCone) { return vec3(0.0f); }

	// assuming they are passed as cos(angle)
	float innerMinusOuter = light.angleInnerCone - light.angleOuterCone;
	// spot light factor for smooth transition
	float spotMark = (cosAngle - light.angleOuterCone) / innerMinusOuter;
	float spotFalloff = smoothstep(0.0f, 1.0f, spotMark);

	if(spotFalloff <= 0.0f) return vec3(0.0f);

	float dst = distance(light.position, position);
	float falloff = 1.0f / (light.attenuation.constant + light.attenuation.linear * dst
					+ light.attenuation.quadratic * dst * dst + 1.0f);   

	if(falloff <= 0.0f) return vec3(0.0f);

	float visibility = 1.0f;

	if(light.shadowingMethod == 2)
	{
		visibility = max(0.0f, TraceShadowCone(position, light.direction, coneShadowAperture, dst));
	}
	else if(light.shadowingMethod == 3)
	{
		vec3 voxelPos = WorldToVoxel(position);
#if USE_OCTREE
		visibility = max(0.0f, SampleOctreeVisibility(volumeDimension, voxelPos));
#else
		visibility = max(0.0f, texture(voxelVisibility, voxelPos).a);
#endif
	}

	if(visibility <= 0.0f) return vec3(0.0f); 

	return BRDF(light, normal, position, albedo, specular) * falloff * spotFalloff * visibility;
}

vec3 CalculateDirectLighting(vec3 position, vec3 normal, vec3 albedo, vec4 specular)
{
	// calculate directional lighting
	vec3 directLighting = vec3(0.0f);

	// calculate lighting for sun light
	{
		Light sunLight;
		sunLight.diffuse = vec3(1.0f);
		sunLight.ambient = vec3(0.001f);
		sunLight.specular = vec3(0.0f);
		sunLight.direction = -global.sunLightDir.xyz;
		sunLight.shadowingMethod = 3;
		directLighting = CalculateDirectional(sunLight, normal, position, albedo, specular);
		directLighting += Ambient(sunLight, albedo);
	}

#if 0
	// calculate lighting for directional lights
	for(int i = 0; i < lightTypeCount[0]; ++i)
	{
		directLighting += CalculateDirectional(directionalLight[i], normal, position, 
										 albedo, specular);
		directLighting += Ambient(directionalLight[i], albedo);
	}

	// calculate lighting for point lights
	for(int i = 0; i < lightTypeCount[1]; ++i)
	{
		directLighting += CalculatePoint(pointLight[i], normal, position, 
								   albedo, specular);
		directLighting += Ambient(pointLight[i], albedo);
	}

	// calculate lighting for spot lights
	for(int i = 0; i < lightTypeCount[2]; ++i) 
	{
		directLighting += CalculateSpot(spotLight[i], normal, position, 
								  albedo, specular);
		directLighting += Ambient(spotLight[i], albedo);
	}
#endif

	return directLighting;
}

vec4 CalculateIndirectLighting(vec3 position, vec3 normal, vec3 albedo, vec4 specular, bool ambientOcclusion)
{
	vec4 specularTrace = vec4(0.0f);
	vec4 diffuseTrace = vec4(0.0f);
	vec3 coneDirection = vec3(0.0f);

	// component greater than zero
	if(any(greaterThan(specular.rgb, specularTrace.rgb)))
	{
		vec3 viewDirection = normalize(cameraPosition.xyz / cameraPosition.w - position);
		vec3 coneDirection = reflect(-viewDirection, normal);
		coneDirection = normalize(coneDirection);
		// specular cone setup, minimum of 1 grad, fewer can severly slow down performance
		float aperture = clamp(tan(HALF_PI * (1.0f - specular.a)), 0.0174533f, PI);
		specularTrace = TraceCone(position, normal, coneDirection, aperture, false);
		specularTrace.rgb *= specular.rgb;
	}

	// component greater than zero
	if(any(greaterThan(albedo, diffuseTrace.rgb)))
	{
		// diffuse cone setup
		const float aperture = 0.57735f;
		vec3 guide = vec3(0.0f, 1.0f, 0.0f);

		if (abs(abs(dot(normal, guide)) - 1.0) < 1e-2)
		{
			guide = vec3(0.0f, 0.0f, 1.0f);
		}

		// Find a tangent and a bitangent
		vec3 right = normalize(guide - dot(normal, guide) * normal);
		vec3 up = cross(right, normal);

		for(int i = 0; i < 6; i++)
		{
			coneDirection = diffuseConeDirections[i].y * normal + diffuseConeDirections[i].x * right + diffuseConeDirections[i].z * up;
			coneDirection = normalize(coneDirection);
			// cumulative result
			diffuseTrace += TraceCone(position, normal, coneDirection, aperture, ambientOcclusion) * diffuseConeWeights[i];
		}

		diffuseTrace.rgb *= albedo;
	}

	vec3 result = bounceStrength * (diffuseTrace.rgb + specularTrace.rgb);

	return vec4(result, ambientOcclusion ? clamp(1.0f - diffuseTrace.a + aoAlpha, 0.0f, 1.0f) : 1.0f);
}

const uint mode = 3;

void main()
{
	vec4 gbuffer0Data = texture(gbuffer0, texCoord);
	// vec4 gbuffer1Data = texture(gbuffer1, texCoord);
	vec4 gbuffer2Data = texture(gbuffer2, texCoord);
	vec4 gbuffer3Data = texture(gbuffer3, texCoord);

	// world-space position
	vec3 position = DecodePosition(gbuffer0Data, texCoord);
	// world-space normal
	vec3 normal = DecodeNormal(gbuffer0Data);
	// normal = normalize(cross(dFdx(position), dFdy(position)));
	// xyz = fragment specular, w = shininess
	vec4 specular = vec4(DecodeSpecularColor(gbuffer3Data), 0.0);
	// fragment albedo
	vec3 baseColor = DecodeBaseColor(gbuffer2Data);
	vec3 albedo = baseColor;
	// fragment emissiviness
	vec3 emissive = vec3(0.0); //texture(gEmissive, texCoord).rgb;
	// lighting cumulatives
	vec3 directLighting = vec3(0.0f);
	vec4 indirectLighting = vec4(0.0f);
	vec3 compositeLighting = vec3(0.0f);

	if(mode == 0) // direct + indirect + ao
	{
		indirectLighting = CalculateIndirectLighting(position, normal, baseColor, specular, true);
		directLighting = CalculateDirectLighting(position, normal, albedo, specular);
	}
	else if(mode == 1) // direct + indirect
	{
		indirectLighting = CalculateIndirectLighting(position, normal, baseColor, specular, false);
		directLighting = CalculateDirectLighting(position, normal, albedo, specular);
	}
	else if(mode == 2) // direct only
	{
		indirectLighting = vec4(0.0f, 0.0f, 0.0f, 1.0f);
		directLighting = CalculateDirectLighting(position, normal, albedo, specular);
	}
	else if(mode == 3) // indirect only
	{
		directLighting = vec3(0.0f);
		// baseColor.rgb = specular.rgb = vec3(1.0f);
		indirectLighting = CalculateIndirectLighting(position, normal, baseColor, specular, false);
	}
	else if(mode == 4) // ambient occlusion only
	{
		directLighting = vec3(0.0f);
		specular = vec4(0.0f);
		indirectLighting = CalculateIndirectLighting(position, normal, baseColor, specular, true);
		indirectLighting.rgb = vec3(1.0f);
	}
	else if(mode == 5) // world normal
	{
		directLighting = normal;
		indirectLighting.rgb = vec3(0.0f);
	}

	// convert indirect to linear space
	// indirectLighting.rgb = pow(indirectLighting.rgb, vec3(gamma));
	// final composite lighting (direct + indirect) * ambient occlusion
	compositeLighting = (directLighting + indirectLighting.rgb) * indirectLighting.a;
	compositeLighting += emissive;
	// -- this could be done in a post-process pass -- 

	// Reinhard tone mapping
	compositeLighting = compositeLighting / (compositeLighting + 1.0f);
	// gamma correction
	// convert to gamma space
	// compositeLighting = pow(compositeLighting, vec3(1.0 / gamma));

	fragColor = vec4(compositeLighting, 1.0f);
}