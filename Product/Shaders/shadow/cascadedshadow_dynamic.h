#ifndef _CSM_DYNAMIC_H_
#define _CSM_DYNAMIC_H_

#include "public.h"
#include "shadow/shadow_sample.h"

layout(binding = BINDING_DYNAMIC_CSM0) uniform sampler2D cascadedDynamicShadowSampler0;
layout(binding = BINDING_DYNAMIC_CSM1) uniform sampler2D cascadedDynamicShadowSampler1;
layout(binding = BINDING_DYNAMIC_CSM2) uniform sampler2D cascadedDynamicShadowSampler2;
layout(binding = BINDING_DYNAMIC_CSM3) uniform sampler2D cascadedDynamicShadowSampler3;

uint CalcDynamicCSMIndex(vec3 worldPos)
{
	// Get cascade index for the current fragment's view position
	vec4 viewPos = camera.view * vec4(worldPos, 1);
	uint cascaded = 0;
	float dis = -viewPos.z;
	for(uint i = 1; i < dynamic_cascaded.cascaded; ++i)
	{
		if(dis > dynamic_cascaded.splitDistance[i - 1])
		{
			cascaded = i;
		}
	}
	return cascaded;
}

float CalcDyanmicCSMShadow(uint cascaded, vec3 worldPos)
{
	vec4 shadowCoord = (biasMat * dynamic_cascaded.light_view_proj[cascaded]) * vec4(worldPos, 1.0);

	vec2 lightRadiusUV = dynamic_cascaded.lightInfo[cascaded].xy;
	float lightZNear = dynamic_cascaded.lightInfo[cascaded].z;
	float lightZFar = dynamic_cascaded.lightInfo[cascaded].w;

	vec2 uv = shadowCoord.xy;
	float z = shadowCoord.z;

	// Compute gradient using ddx/ddy before any branching
	vec2 dz_duv = DepthGradient(uv, z);
	// Eye-space z from the light's point of view
	float zEye = -(dynamic_cascaded.light_view[cascaded] * vec4(worldPos, 1.0)).z;

	const float ZNEAR = 1.0;
	float translateZ = ZNEAR - lightZNear;
	lightZNear += translateZ;
	lightZFar += translateZ;
	zEye += translateZ;

	float shadow = 1.0;

	if (cascaded == 0)
	{
		shadow = SoftShadow(cascadedDynamicShadowSampler0,
			lightRadiusUV, lightZNear, lightZFar,
			uv, z, dz_duv, zEye);
	}
	else if (cascaded == 1)
	{
		shadow = SoftShadow(cascadedDynamicShadowSampler1,
			lightRadiusUV, lightZNear, lightZFar,
			uv, z, dz_duv, zEye);
	}
	else if (cascaded == 2)
	{
		shadow = SoftShadow(cascadedDynamicShadowSampler2,
			lightRadiusUV, lightZNear, lightZFar,
			uv, z, dz_duv, zEye);
	}
	else if (cascaded == 3)
	{
		shadow = SoftShadow(cascadedDynamicShadowSampler3,
			lightRadiusUV, lightZNear, lightZFar,
			uv, z, dz_duv, zEye);
	}

	return 1.0 - shadow;
}

vec4 CalcDynamicCSM(vec3 worldPos)
{
	vec4 outColor;
	uint cascaded = CalcDynamicCSMIndex(worldPos);
	float shadow = CalcDyanmicCSMShadow(cascaded, worldPos);
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