#include "public.h"
#include "voxel/clipmap/voxel_clipmap_common.h"

layout(location = 0) out vec4 fragColor;

layout(location = 0) in vec2 texCoord;

layout(binding = VOXEL_CLIPMAP_BINDING_GBUFFER_NORMAL) uniform sampler2D gNormal;
layout(binding = VOXEL_CLIPMAP_BINDING_GBUFFER_POSITION) uniform sampler2D gPosition;
layout(binding = VOXEL_CLIPMAP_BINDING_GBUFFER_ALBEDO) uniform sampler2D gAlbedo;
layout(binding = VOXEL_CLIPMAP_BINDING_GBUFFER_SPECULAR) uniform sampler2D gSpecular;
// layout(binding = VOXEL_CLIPMAP_BINDING_GBUFFER_EMISSIVE) uniform sampler2D gEmissive;

layout(binding = VOXEL_CLIPMAP_BINDING_VISIBILITY) uniform sampler3D voxelVisibility;
layout(binding = VOXEL_CLIPMAP_BINDING_RADIANCE) uniform sampler3D voxelRadiance;

vec4 cameraPosition = camera.viewInv * vec4(0.0, 0.0, 0.0, 1.0);

float baseExtent = voxel_clipmap.region_max_and_extent[0].w;
float baseVoxelSize = voxel_clipmap.region_min_and_voxelsize[0].w;
float lastVoxelSize = voxel_clipmap.region_min_and_voxelsize[levelCount - 1].w;
float lastExtent = voxel_clipmap.region_max_and_extent[levelCount - 1].w;

vec3 lastRegionMin = voxel_clipmap.region_min_and_voxelsize[levelCount - 1].xyz;
vec3 lastRegionMax = voxel_clipmap.region_max_and_extent[levelCount - 1].xyz;

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

vec4 SampleClipmap(vec3 posW, uint clipmapLevel, uvec3 visibleFace, vec3 weight)
{
	float extent = voxel_clipmap.region_max_and_extent[clipmapLevel].w;
	vec3 samplePosX = WorldPositionToSampleCoord(posW, extent, clipmapLevel, levelCount, visibleFace.x, 6);
	vec3 samplePosY = WorldPositionToSampleCoord(posW, extent, clipmapLevel, levelCount, visibleFace.y, 6);
	vec3 samplePosZ = WorldPositionToSampleCoord(posW, extent, clipmapLevel, levelCount, visibleFace.z, 6);
	return clamp(texture(voxelRadiance, samplePosX) * weight.x +
				 texture(voxelRadiance, samplePosX) * weight.y +
				 texture(voxelRadiance, samplePosZ) * weight.z, 0.0, 1.0);
}

vec4 SampleClipmapLinearly(vec3 posW, float curLevel, uvec3 visibleFace, vec3 weight)
{
	uint lowerLevel = uint(floor(curLevel));
	uint upperLevel = uint(ceil(curLevel));

	vec4 lowSample = SampleClipmap(posW, lowerLevel, visibleFace, weight);

	if (lowerLevel == upperLevel)
		return lowSample;

	vec4 highSample = SampleClipmap(posW, upperLevel, visibleFace, weight);

	return mix(lowSample, highSample, fract(curLevel));
}

bool IntersectRayWithWorldAABB(vec3 ro, vec3 rd, out float enter, out float leave)
{
	vec3 tempMin = (lastRegionMin - ro) / rd; 
	vec3 tempMax = (lastRegionMax - ro) / rd;

	vec3 v3Max = max (tempMax, tempMin);
	vec3 v3Min = min (tempMax, tempMin);

	leave = min (v3Max.x, min (v3Max.y, v3Max.z));
	enter = max (max (v3Min.x, 0.0), max (v3Min.y, v3Min.z));    

	return leave > enter;
}

float GetMinLevel(vec3 posW)
{
	float distanceToCenter = length(cameraPosition.xyz - posW);
	float minRadius = baseExtent * 0.5;
	float minLevel = log2(distanceToCenter / minRadius);  
	minLevel = max(0.0, minLevel);

	float radius = minRadius * exp2(ceil(minLevel));
	float f = distanceToCenter / radius;

	// Smoothly transition from current level to the next level
	float transitionStart = 0.5;
	float c = 1.0 / (1.0 - transitionStart);
	
	return f > transitionStart ? ceil(minLevel) + (f - transitionStart) * c : ceil(minLevel);
}

vec4 TraceCone(vec3 position, vec3 normal, vec3 direction, float aperture, bool traceOcclusion)
{
	uvec3 visibleFace;
	visibleFace.x = (direction.x < 0.0) ? 0 : 1;
	visibleFace.y = (direction.y < 0.0) ? 2 : 3;
	visibleFace.z = (direction.z < 0.0) ? 4 : 5;
	traceOcclusion = traceOcclusion && aoAlpha < 1.0f;

	float startLevel = GetMinLevel(position);
	// weight per axis for aniso sampling
	vec3 weight = direction * direction;
	// move further to avoid self collision
	float dst = baseVoxelSize * exp2(startLevel);
	vec3 startPosition = position + normal * dst;
	// final results
	vec4 coneSample = vec4(0.0f);
	vec4 anisoSample = vec4(0.0f);
	float occlusion = 0.0f;

	float curSegmentLength = dst;
	float opacity = 0;

	float maxDistance = lastExtent * maxTracingDistanceGlobal;
	float falloff = 0.5f * aoFalloff / lastExtent;
	// out of boundaries check
	float enter = 0.0; float leave = 0.0;

	if(!IntersectRayWithWorldAABB(position, direction, enter, leave))
	{
		coneSample.a = 1.0f;
	}

	float c = lastVoxelSize * 0.5; // Error correction constant
	vec3 correctedRegionMin = lastRegionMin + c;
	vec3 correctedRegionMax = lastRegionMax - c;

	while (coneSample.a < 1.0f && dst <= maxDistance)
	{
		vec3 conePosition = startPosition + direction * dst;

		// outisde bounds
		if (checkBoundaries > 0 && (conePosition.x < correctedRegionMin.x || conePosition.y < correctedRegionMin.y || conePosition.z < correctedRegionMin.z
			|| conePosition.x >= correctedRegionMax.x || conePosition.y >= correctedRegionMax.y || conePosition.z >= correctedRegionMax.z))
		{
			break;
		}

		// cone expansion and respective mip level based on diameter
		float diameter = 2.0f * aperture * dst;

		float distanceToCenter = distance(cameraPosition.xyz, conePosition);
		float minLevel = ceil(log2(distanceToCenter * 2 / baseExtent));

		float mipLevel = log2(diameter / baseVoxelSize);
		mipLevel = min(max(max(mipLevel, startLevel), minLevel), levelCount - 1);

		// Radiance correction
		float correctionQuotient = curSegmentLength / (baseVoxelSize * exp2(mipLevel));

		// aniso sampling
		anisoSample = SampleClipmapLinearly(conePosition, mipLevel, visibleFace, weight);
		opacity = anisoSample.a;
		opacity = clamp(1.0 - pow(1.0 - opacity, correctionQuotient), 0.0, 1.0);
		anisoSample.rgb = anisoSample.rgb * correctionQuotient;
		anisoSample.a = opacity;

		coneSample += (1.0f - coneSample.a) * anisoSample;
		if(traceOcclusion && occlusion < 1.0)
		{
			occlusion += ((1.0f - occlusion) * opacity) / (1.0f + falloff * diameter);
		}

		float dstLast = dst;
		// move further into volume
		dst += diameter * samplingFactor;
		// dst += baseVoxelSize * samplingFactor;
		curSegmentLength = (dst - dstLast);
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

	float startLevel = GetMinLevel(position);

	// weight per axis for aniso sampling
	vec3 weight = direction * direction;
	// move further to avoid self collision
	float dst = baseVoxelSize * exp2(startLevel);;
	vec3 startPosition = position + direction * dst;

	// final results
	float visibility = 0.0f;
	vec4 anisoSample = vec4(0.0f);
	float k = exp2(7.0f * coneShadowTolerance);
	// cone will only trace the needed distance
	float maxDistance = lastExtent * maxTracingDistance;
	// out of boundaries check
	float enter = 0.0; float leave = 0.0;

	if(!IntersectRayWithWorldAABB(position, direction, enter, leave))
	{
		visibility = 1.0f;
	}

	float c = lastVoxelSize * 0.5; // Error correction constant
	vec3 correctedRegionMin = lastRegionMin + c;
	vec3 correctedRegionMax = lastRegionMax - c;

	while(visibility < 1.0f && dst <= maxDistance)
	{
		vec3 conePosition = startPosition + direction * dst;

		// outisde bounds
		if (checkBoundaries > 0 && (conePosition.x < correctedRegionMin.x || conePosition.y < correctedRegionMin.y || conePosition.z < correctedRegionMin.z
			|| conePosition.x >= correctedRegionMax.x || conePosition.y >= correctedRegionMax.y || conePosition.z >= correctedRegionMax.z))
		{
			break;
		}

		// cone expansion and respective mip level based on diameter
		float diameter = 2.0f * aperture * dst;

		float distanceToCenter = distance(cameraPosition.xyz, conePosition);
		float minLevel = ceil(log2(distanceToCenter * 2 / baseExtent));

		float mipLevel = log2(diameter / baseVoxelSize);
		mipLevel = min(max(max(mipLevel, startLevel), minLevel), levelCount - 1);

		// aniso sampling
		anisoSample = SampleClipmapLinearly(conePosition, mipLevel, visibleFace, weight);

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
		visibility = max(0.0f, TraceShadowCone(position, light.direction, coneShadowAperture, 1.0f));
	}
	else if(light.shadowingMethod == 3)
	{
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
	// world-space position
	vec3 position = texture(gPosition, texCoord).xyz;
	// world-space normal
	vec3 normal = normalize(texture(gNormal, texCoord).xyz);
	// normal = normalize(cross(dFdx(position), dFdy(position)));
	// xyz = fragment specular, w = shininess
	vec4 specular = texture(gSpecular, texCoord);
	// fragment albedo
	vec3 baseColor = texture(gAlbedo, texCoord).rgb;
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
	else if(mode == 5)
	{
		directLighting = 0.5 * (normal + vec3(1.0));
		indirectLighting.rgb = vec3(0.0f);
	}

	// convert indirect to linear space
	// indirectLighting.rgb = pow(indirectLighting.rgb, vec3(gamma));
	// final composite lighting (direct + indirect) * ambient occlusion
	compositeLighting = (directLighting + indirectLighting.rgb) * indirectLighting.a;
	compositeLighting += emissive;
	// -- this could be done in a post-process pass -- 

	// Reinhard tone mapping
	// compositeLighting = compositeLighting / (compositeLighting + 1.0f);
	// gamma correction
	// convert to gamma space
	// compositeLighting = pow(compositeLighting, vec3(1.0 / gamma));

	fragColor = vec4(compositeLighting, 1.0f);
}