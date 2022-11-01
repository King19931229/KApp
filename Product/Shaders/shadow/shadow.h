#ifndef _SHADOW_H_
#define _SHADOW_H_

#include "public.h"
#include "shadow/shadow_sample.h"

layout(binding = BINDING_CSM0) uniform sampler2D cascadedShadowSampler0;
layout(binding = BINDING_CSM1) uniform sampler2D cascadedShadowSampler1;
layout(binding = BINDING_CSM2) uniform sampler2D cascadedShadowSampler2;
layout(binding = BINDING_CSM3) uniform sampler2D cascadedShadowSampler3;

const bool pcss_shadow = false;
const bool light_size_shadow = false;

float SoftShadow(sampler2D shadowMap,
	vec2 lightRadiusUV,
	float lightZNear, float lightZFar,
	vec2 uv, float z, vec2 dz_duv, float zEye)
{

	if(light_size_shadow)
	{
		return pcss_shadow ? PcssShadow(shadowMap,
			lightRadiusUV, lightZNear, lightZFar,
			uv, z, dz_duv, zEye) : PcfShadow(shadowMap,
			lightRadiusUV, lightZNear, lightZFar,
			uv, z, dz_duv, zEye);
	}
	else
	{
		return Pcf_4x4(shadowMap, uv, z);
	}
}

uint CalcCSMIndex(vec3 viewPos)
{
	// Get cascade index for the current fragment's view position
	uint cascaded = 0;
	for(uint i = 0; i < cascaded_shadow.cascaded - 1; ++i)
	{
		if(viewPos.z < cascaded_shadow.frustum[i])
		{
			cascaded = i + 1;
		}
	}
	return cascaded;

}

vec4 BlendCSMColor(uint cascaded, vec4 inColor)
{
	vec4 outColor = inColor;
	switch(cascaded)
	{
		case 0 : 
			outColor.rgb *= vec3(1.0f, 0.25f, 0.25f);
			break;
		case 1 : 
			outColor.rgb *= vec3(0.25f, 1.0f, 0.25f);
			break;
		case 2 : 
			outColor.rgb *= vec3(0.25f, 0.25f, 1.0f);
			break;
		case 3 : 
			outColor.rgb *= vec3(1.0f, 1.0f, 0.25f);
			break;
	}
	return outColor;
}

float CalcCSMShadow(uint cascaded, vec3 worldPos)
{
	vec4 shadowCoord = (biasMat * cascaded_shadow.light_view_proj[cascaded]) * vec4(worldPos, 1.0);

	vec2 lightRadiusUV = cascaded_shadow.lightInfo[cascaded].xy;
	float lightZNear = cascaded_shadow.lightInfo[cascaded].z;
	float lightZFar = cascaded_shadow.lightInfo[cascaded].w;

	vec2 uv = shadowCoord.xy;
	float z = shadowCoord.z;

	// Compute gradient using ddx/ddy before any branching
	vec2 dz_duv = DepthGradient(uv, z);
	// Eye-space z from the light's point of view
	float zEye = -(cascaded_shadow.light_view[cascaded] * vec4(worldPos, 1.0)).z;

	const float ZNEAR = 1.0;
	float translateZ = ZNEAR - lightZNear;
	lightZNear += translateZ;
	lightZFar += translateZ;
	zEye += translateZ;

	float shadow = 1.0;

	if(cascaded == 0)
	{
		shadow = SoftShadow(cascadedShadowSampler0,
			lightRadiusUV, lightZNear, lightZFar,
			uv, z, dz_duv, zEye);
	}
	else if(cascaded == 1)
	{
		shadow = SoftShadow(cascadedShadowSampler1,
			lightRadiusUV, lightZNear, lightZFar,
			uv, z, dz_duv, zEye);
	}
	else if(cascaded == 2)
	{
		shadow = SoftShadow(cascadedShadowSampler2,
			lightRadiusUV, lightZNear, lightZFar,
			uv, z, dz_duv, zEye);
	}
	else if(cascaded == 3)
	{
		shadow = SoftShadow(cascadedShadowSampler3,
			lightRadiusUV, lightZNear, lightZFar,
			uv, z, dz_duv, zEye);
	}

	return 1.0 - shadow;
}

const bool debug_layer = false;

vec4 CalcCSM(vec3 viewPos, vec3 worldPos)
{
	vec4 outColor;
	uint cascaded = CalcCSMIndex(viewPos);
	float shadow = CalcCSMShadow(cascaded, worldPos);
	if(debug_layer)
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