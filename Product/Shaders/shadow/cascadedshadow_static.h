#ifndef _CSM_STATIC_H_
#define _CSM_STATIC_H_

#include "public.h"
#include "shadow/shadow_sample.h"

layout(binding = BINDING_STATIC_CSM0) uniform sampler2D cascadedStaticShadowSampler0;
layout(binding = BINDING_STATIC_CSM1) uniform sampler2D cascadedStaticShadowSampler1;
layout(binding = BINDING_STATIC_CSM2) uniform sampler2D cascadedStaticShadowSampler2;
layout(binding = BINDING_STATIC_CSM3) uniform sampler2D cascadedStaticShadowSampler3;

uint CalcStaticCSMIndex(vec3 worldPos)
{
	// Get cascade index for the current fragment's view position
	vec3 offset = worldPos - static_cascaded.center.xyz;
	uint cascaded = 0;
	float dis = max(max(abs(offset.x), abs(offset.y)), abs(offset.z));
	for(uint i = 0; i < static_cascaded.cascaded; ++i)
	{
		if(dis > static_cascaded.area[i - 1])
		{
			cascaded = i;
		}
	}
	return cascaded;
}

float CalcStaticCSMShadow(uint cascaded, vec3 worldPos)
{
	const mat4 biasMat = mat4(
		0.5, 0.0, 0.0, 0.0,
		0.0, 0.5, 0.0, 0.0,
		0.0, 0.0, 1.0, 0.0,
		0.5, 0.5, 0.0, 1.0);

	vec4 shadowCoord = (biasMat * static_cascaded.light_view_proj[cascaded]) * vec4(worldPos, 1.0);

	vec2 lightRadiusUV = static_cascaded.lightInfo[cascaded].xy;
	float lightZNear = static_cascaded.lightInfo[cascaded].z;
	float lightZFar = static_cascaded.lightInfo[cascaded].w;

	vec2 uv = shadowCoord.xy;
	float z = shadowCoord.z;

	// Compute gradient using ddx/ddy before any branching
	vec2 dz_duv = DepthGradient(uv, z);
	// Eye-space z from the light's point of view
	float zEye = -(static_cascaded.light_view[cascaded] * vec4(worldPos, 1.0)).z;

	const float ZNEAR = 1.0;
	float translateZ = ZNEAR - lightZNear;
	lightZNear += translateZ;
	lightZFar += translateZ;
	zEye += translateZ;

	float shadow = 1.0;

	if(cascaded == 0)
	{
		shadow = SoftShadow(cascadedStaticShadowSampler0,
			lightRadiusUV, lightZNear, lightZFar,
			uv, z, dz_duv, zEye);
	}
	else if(cascaded == 1)
	{
		shadow = SoftShadow(cascadedStaticShadowSampler1,
			lightRadiusUV, lightZNear, lightZFar,
			uv, z, dz_duv, zEye);
	}
	else if(cascaded == 2)
	{
		shadow = SoftShadow(cascadedStaticShadowSampler2,
			lightRadiusUV, lightZNear, lightZFar,
			uv, z, dz_duv, zEye);
	}
	else if(cascaded == 3)
	{
		shadow = SoftShadow(cascadedStaticShadowSampler3,
			lightRadiusUV, lightZNear, lightZFar,
			uv, z, dz_duv, zEye);
	}

	return 1.0 - shadow;
}

vec4 CalcStaticCSM(vec3 worldPos)
{
	vec4 outColor;
	uint cascaded = CalcStaticCSMIndex(worldPos);
	float shadow = CalcStaticCSMShadow(cascaded, worldPos);
	if (debug_layer)
	{
		outColor = shadow * BlendCSMColor(cascaded, vec4(1.0f));
	}
	else
	{
		outColor = vec4(shadow);
	}
	return outColor;
}

float CalcStaticCSMHardShadow(uint cascaded, vec3 worldPos)
{
	const mat4 biasMat = mat4(
		0.5, 0.0, 0.0, 0.0,
		0.0, 0.5, 0.0, 0.0,
		0.0, 0.0, 1.0, 0.0,
		0.5, 0.5, 0.0, 1.0);

	vec4 shadowCoord = (biasMat * static_cascaded.light_view_proj[cascaded]) * vec4(worldPos, 1.0);
	vec2 uv = shadowCoord.xy;
	float z = shadowCoord.z;

	float shadow = 1.0;

	if(cascaded == 0)
	{
		shadow = HardShadow(cascadedStaticShadowSampler0, uv, z);
	}
	else if(cascaded == 1)
	{
		shadow = HardShadow(cascadedStaticShadowSampler1, uv, z);
	}
	else if(cascaded == 2)
	{
		shadow = HardShadow(cascadedStaticShadowSampler2, uv, z);
	}
	else if(cascaded == 3)
	{
		shadow = HardShadow(cascadedStaticShadowSampler3, uv, z);
	}

	return 1.0 - shadow;
}

vec4 CalcStaticCSMHard(vec3 worldPos)
{
	vec4 outColor;
	uint cascaded = CalcStaticCSMIndex(worldPos);
	float shadow = CalcStaticCSMHardShadow(cascaded, worldPos);
	if (debug_layer)
	{
		outColor = shadow * BlendCSMColor(cascaded, vec4(1.0f));
	}
	else
	{
		outColor = vec4(shadow);
	}
	return outColor;
}

#endif