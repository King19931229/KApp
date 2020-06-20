#ifndef SHADOW_H
#define SHADOW_H

#include "public.h"

layout(binding = BINDING_CSM0) uniform sampler2D cascadedShadowSampler0;
layout(binding = BINDING_CSM1) uniform sampler2D cascadedShadowSampler1;
layout(binding = BINDING_CSM2) uniform sampler2D cascadedShadowSampler2;
layout(binding = BINDING_CSM3) uniform sampler2D cascadedShadowSampler3;

const float ambient = 0.4;

const mat4 biasMat = mat4(
	0.5, 0.0, 0.0, 0.0,
	0.0, 0.5, 0.0, 0.0,
	0.0, 0.0, 1.0, 0.0,
	0.5, 0.5, 0.0, 1.0 );

const vec2 poisson16[] = vec2[](
	vec2( -0.94201624,  -0.39906216 ),
	vec2(  0.94558609,  -0.76890725 ),
	vec2( -0.094184101, -0.92938870 ),
	vec2(  0.34495938,   0.29387760 ),
	vec2( -0.91588581,   0.45771432 ),
	vec2( -0.81544232,  -0.87912464 ),
	vec2( -0.38277543,   0.27676845 ),
	vec2(  0.97484398,   0.75648379 ),
	vec2(  0.44323325,  -0.97511554 ),
	vec2(  0.53742981,  -0.47373420 ),
	vec2( -0.26496911,  -0.41893023 ),
	vec2(  0.79197514,   0.19090188 ),
	vec2( -0.24188840,   0.99706507 ),
	vec2( -0.81409955,   0.91437590 ),
	vec2(  0.19984126,   0.78641367 ),
	vec2(  0.14383161,  -0.14100790 )
);

float zClipToEye(float lightZNear, float lightZFar, float z)
{
	return lightZFar * lightZNear / (lightZFar - z * (lightZFar - lightZNear));
}

// Using similar triangles from the surface point to the area light
vec2 searchRegionRadiusUV(vec2 lightRadiusUV, float lightZNear, float zWorld)
{
	return lightRadiusUV * (zWorld - lightZNear) / zWorld;
}

// Using similar triangles between the area light, the blocking plane and the surface point
vec2 penumbraRadiusUV(vec2 lightRadiusUV, float zReceiver, float zBlocker)
{
	return lightRadiusUV * (zReceiver - zBlocker) / zBlocker;
}

// Project UV size to the near plane of the light
vec2 projectToLightUV(float lightZNear, vec2 sizeUV, float zWorld)
{
	return sizeUV * lightZNear / zWorld;
}

// Derivatives of light-space depth with respect to texture2D coordinates
vec2 depthGradient(vec2 uv, float z)
{
	vec2 dz_duv = vec2(0.0, 0.0);

	vec3 duvdist_dx = dFdx(vec3(uv,z));
	vec3 duvdist_dy = dFdy(vec3(uv,z));

	dz_duv.x = duvdist_dy.y * duvdist_dx.z;
	dz_duv.x -= duvdist_dx.y * duvdist_dy.z;

	dz_duv.y = duvdist_dx.x * duvdist_dy.z;
	dz_duv.y -= duvdist_dy.x * duvdist_dx.z;

	float det = (duvdist_dx.x * duvdist_dy.y) - (duvdist_dx.y * duvdist_dy.x);
	dz_duv /= det;

	return dz_duv;
}

float biasedZ(float z0, vec2 dz_duv, vec2 offset)
{
	return z0 + dot(dz_duv, offset);
}

float borderDepthTexture(sampler2D tex, vec2 uv)
{
	return ((uv.x <= 1.0) && (uv.y <= 1.0) &&
	 (uv.x >= 0.0) && (uv.y >= 0.0)) ? textureLod(tex, uv, 0.0).x : 1.0;
}

float borderPCFTexture(sampler2D tex, vec3 uvz)
{
	if((uvz.x <= 1.0) && (uvz.y <= 1.0) && (uvz.x >= 0.0) && (uvz.y >= 0.0))
	{
		return textureLod(tex, uvz.xy, 0).x <= uvz.z ? 1.0 : 0.0; 
	}
	return ((uvz.z <= 1.0) ? 1.0 : 0.0);
}

// Returns average blocker depth in the search region, as well as the number of found blockers.
// Blockers are defined as shadow-map samples between the surface point and the light.
void findBlocker(
	out float accumBlockerDepth, 
	out float numBlockers,
	out float maxBlockers,
	sampler2D shadowMap,
	vec2 uv,
	float z0,
	vec2 dz_duv,
	vec2 searchRegionRadiusUV)
{
	accumBlockerDepth = 0.0;
	numBlockers = 0.0;
	maxBlockers = 16.0;

	for (int i = 0; i < 16; ++i)
	{
		vec2 offset = poisson16[i] * searchRegionRadiusUV;
		float shadowMapDepth = borderDepthTexture(shadowMap, uv + offset);
		float z = biasedZ(z0, dz_duv, offset);
		if (shadowMapDepth < z)
		{
			accumBlockerDepth += shadowMapDepth;
			numBlockers++;
		}
	}
}

float pcf_4x4(sampler2D shadowMap, vec2 uv, float z0)
{
	vec2 vInvShadowMapWH = vec2(1.0) / vec2(textureSize(shadowMap, 0));
	vec2 TexelPos = uv / vInvShadowMapWH - vec2(0.5, 0.5);	// bias to be consistent with texture filtering hardware
	vec2 Fraction = fract(TexelPos);
	vec2 TexelCenter = floor(TexelPos) + vec2(0.5, 0.5);	// bias to get reliable texel center content
	vec2 Sample00TexelCenter = (TexelCenter - vec2(1, 1)) * vInvShadowMapWH;
	vec4 litZ = vec4(z0);

	vec4 Values0, Values1, Values2, Values3;// Valuesi in row i from left to right: x,y,z,w
	Values0.x  = borderDepthTexture(shadowMap, (Sample00TexelCenter + vec2(0,0) * vInvShadowMapWH));
	Values0.y  = borderDepthTexture(shadowMap, (Sample00TexelCenter + vec2(1,0) * vInvShadowMapWH));
	Values0.z  = borderDepthTexture(shadowMap, (Sample00TexelCenter + vec2(2,0) * vInvShadowMapWH));
	Values0.w  = borderDepthTexture(shadowMap, (Sample00TexelCenter + vec2(3,0) * vInvShadowMapWH));
	Values0 = step(Values0, litZ.xxxx);
	Values1.x = borderDepthTexture(shadowMap, (Sample00TexelCenter + vec2(0,1) * vInvShadowMapWH));
	Values1.y = borderDepthTexture(shadowMap, (Sample00TexelCenter + vec2(1,1) * vInvShadowMapWH));
	Values1.z = borderDepthTexture(shadowMap, (Sample00TexelCenter + vec2(2,1) * vInvShadowMapWH));
	Values1.w = borderDepthTexture(shadowMap, (Sample00TexelCenter + vec2(3,1) * vInvShadowMapWH));
	Values1 = step(Values1, litZ.xxxx);
	Values2.x = borderDepthTexture(shadowMap, (Sample00TexelCenter + vec2(0,2) * vInvShadowMapWH));
	Values2.y = borderDepthTexture(shadowMap, (Sample00TexelCenter + vec2(1,2) * vInvShadowMapWH));
	Values2.z = borderDepthTexture(shadowMap, (Sample00TexelCenter + vec2(2,2) * vInvShadowMapWH));
	Values2.w = borderDepthTexture(shadowMap, (Sample00TexelCenter + vec2(3,2) * vInvShadowMapWH));
	Values2 = step(Values2, litZ.xxxx);
	Values3.x = borderDepthTexture(shadowMap, (Sample00TexelCenter + vec2(0,3) * vInvShadowMapWH));
	Values3.y = borderDepthTexture(shadowMap, (Sample00TexelCenter + vec2(1,3) * vInvShadowMapWH));
	Values3.z = borderDepthTexture(shadowMap, (Sample00TexelCenter + vec2(2,3) * vInvShadowMapWH));
	Values3.w = borderDepthTexture(shadowMap, (Sample00TexelCenter + vec2(3,3) * vInvShadowMapWH));
	Values3 = step(Values3, litZ.xxxx);

	vec2 VerticalLerp00 = mix(vec2(Values0.x, Values1.x), vec2(Values0.y, Values1.y), Fraction.xx);
	float PCFResult00 = mix(VerticalLerp00.x, VerticalLerp00.y, Fraction.y);
	vec2 VerticalLerp10 = mix(vec2(Values0.y, Values1.y), vec2(Values0.z, Values1.z), Fraction.xx);
	float PCFResult10 = mix(VerticalLerp10.x, VerticalLerp10.y, Fraction.y);
	vec2 VerticalLerp20 = mix(vec2(Values0.z, Values1.z), vec2(Values0.w, Values1.w), Fraction.xx);
	float PCFResult20 = mix(VerticalLerp20.x, VerticalLerp20.y, Fraction.y);

	vec2 VerticalLerp01 = mix(vec2(Values1.x, Values2.x), vec2(Values1.y, Values2.y), Fraction.xx);
	float PCFResult01 = mix(VerticalLerp01.x, VerticalLerp01.y, Fraction.y);
	vec2 VerticalLerp11 = mix(vec2(Values1.y, Values2.y), vec2(Values1.z, Values2.z), Fraction.xx);
	float PCFResult11 = mix(VerticalLerp11.x, VerticalLerp11.y, Fraction.y);
	vec2 VerticalLerp21 = mix(vec2(Values1.z, Values2.z), vec2(Values1.w, Values2.w), Fraction.xx);
	float PCFResult21 = mix(VerticalLerp21.x, VerticalLerp21.y, Fraction.y);

	vec2 VerticalLerp02 = mix(vec2(Values2.x, Values3.x), vec2(Values2.y, Values3.y), Fraction.xx);
	float PCFResult02 = mix(VerticalLerp02.x, VerticalLerp02.y, Fraction.y);
	vec2 VerticalLerp12 = mix(vec2(Values2.y, Values3.y), vec2(Values2.z, Values3.z), Fraction.xx);
	float PCFResult12 = mix(VerticalLerp12.x, VerticalLerp12.y, Fraction.y);
	vec2 VerticalLerp22 = mix(vec2(Values2.z, Values3.z), vec2(Values2.w, Values3.w), Fraction.xx);
	float PCFResult22 = mix(VerticalLerp22.x, VerticalLerp22.y, Fraction.y);

	float inShadow = (PCFResult00 + PCFResult10 + PCFResult20 + PCFResult01 + PCFResult11 + PCFResult21 + PCFResult02 + PCFResult12 + PCFResult22) * .11111;
	return inShadow;
}

float pcf_2x2(sampler2D shadowMap, vec2 uv, float z0)
{
	vec2 vInvShadowMapWH = vec2(1.0) / vec2(textureSize(shadowMap, 0));
	vec2 fc = fract(uv / vInvShadowMapWH - 0.5);
	vec2 tc0 = uv - (fc - vec2(0.5)) * vInvShadowMapWH;
	vec2 tc1 = tc0 + vec2(vInvShadowMapWH.x, 0);
	vec2 tc2 = tc0 + vec2(0, vInvShadowMapWH.y);
	vec2 tc3 = tc1 + vec2(0, vInvShadowMapWH.y);

	vec4 litZ = vec4(z0);

	vec4 sampleDepth;
	sampleDepth.x = borderDepthTexture(shadowMap, tc0);
	sampleDepth.y = borderDepthTexture(shadowMap, tc1);
	sampleDepth.z = borderDepthTexture(shadowMap, tc2);
	sampleDepth.w = borderDepthTexture(shadowMap, tc3);

	vec4 InShadow;
	InShadow.x  = float(litZ.x > sampleDepth.x);
	InShadow.y  = float(litZ.x > sampleDepth.y);
	InShadow.z  = float(litZ.x > sampleDepth.z);
	InShadow.w  = float(litZ.x > sampleDepth.w);

	float inShadow01 = mix(InShadow.x, InShadow.y, fc.x);
	float inShadow23 = mix(InShadow.z, InShadow.w, fc.x);
	float inShadow = mix(inShadow01, inShadow23, fc.y);
	return inShadow;
}

// Performs PCF filtering on the shadow map using multiple taps in the filter region.
float pcfFilter(sampler2D shadowMap, vec2 uv, float z0, vec2 dz_duv, vec2 filterRadiusUV)
{
	float sum = 0.0;
	for (int i = 0; i < 16; ++i)
	{
		vec2 offset = poisson16[i] * filterRadiusUV;
		float z = biasedZ(z0, dz_duv, offset);
		sum += pcf_2x2(shadowMap, uv + offset, z);
	}
	return sum / 16.0;
}

float pcssShadow(sampler2D shadowMap,
	vec2 lightRadiusUV,
	float lightZNear, float lightZFar,
	vec2 uv, float z, vec2 dz_duv, float zEye)
{
	// ------------------------
	// STEP 1: blocker search
	// ------------------------
	float accumBlockerDepth, numBlockers, maxBlockers;
	vec2 searchRegionRadiusUV = searchRegionRadiusUV(lightRadiusUV, lightZNear, zEye);
	findBlocker(accumBlockerDepth, numBlockers, maxBlockers, shadowMap, uv, z, dz_duv, searchRegionRadiusUV);

	// Early out if not in the penumbra
	if (numBlockers == 0.0)
		return 0.0;

	// ------------------------
	// STEP 2: penumbra size
	// ------------------------
	float avgBlockerDepth = accumBlockerDepth / numBlockers;
	float avgBlockerDepthWorld = zClipToEye(lightZNear, lightZFar, avgBlockerDepth);
	vec2 penumbraRadius = penumbraRadiusUV(lightRadiusUV, zEye, avgBlockerDepthWorld);
	vec2 filterRadius = projectToLightUV(lightZNear, penumbraRadius, zEye);

	// ------------------------
	// STEP 3: filtering
	// ------------------------
	return pcfFilter(shadowMap, uv, z, dz_duv, filterRadius);
}

float pcfShadow(sampler2D shadowMap,
	vec2 lightRadiusUV,
	float lightZNear, float lightZFar,
	vec2 uv, float z, vec2 dz_duv, float zEye)
{
	vec2 filterRadius = 0.1 * lightRadiusUV;
	return pcfFilter(shadowMap, uv, z, dz_duv, filterRadius);
}

const bool pcss_shadow = false;
const bool light_size_shadow = false;

float softShadow(sampler2D shadowMap,
	vec2 lightRadiusUV,
	float lightZNear, float lightZFar,
	vec2 uv, float z, vec2 dz_duv, float zEye)
{

	if(light_size_shadow)
	{
		return pcss_shadow ? pcssShadow(shadowMap,
			lightRadiusUV, lightZNear, lightZFar,
			uv, z, dz_duv, zEye) : pcfShadow(shadowMap,
			lightRadiusUV, lightZNear, lightZFar,
			uv, z, dz_duv, zEye);
	}
	else
	{
		return pcf_4x4(shadowMap, uv, z);
	}
}

uint calcCSMIndex(vec3 viewPos)
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

vec4 blendCSMColor(uint cascaded, vec4 inColor)
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

float calcCSMShadow(uint cascaded, vec3 worldPos)
{
	vec4 shadowCoord = (biasMat * cascaded_shadow.light_view_proj[cascaded]) * vec4(worldPos, 1.0);

	vec2 lightRadiusUV = cascaded_shadow.lightInfo[cascaded].xy;
	float lightZNear = cascaded_shadow.lightInfo[cascaded].z;
	float lightZFar = cascaded_shadow.lightInfo[cascaded].w;

	vec2 uv = shadowCoord.xy;
	float z = shadowCoord.z;

	// Compute gradient using ddx/ddy before any branching
	vec2 dz_duv = depthGradient(uv, z);
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
		shadow = softShadow(cascadedShadowSampler0,
			lightRadiusUV, lightZNear, lightZFar,
			uv, z, dz_duv, zEye);
	}
	else if(cascaded == 1)
	{
		shadow = softShadow(cascadedShadowSampler1,
			lightRadiusUV, lightZNear, lightZFar,
			uv, z, dz_duv, zEye);
	}
	else if(cascaded == 2)
	{
		shadow = softShadow(cascadedShadowSampler2,
			lightRadiusUV, lightZNear, lightZFar,
			uv, z, dz_duv, zEye);
	}
	else if(cascaded == 3)
	{
		shadow = softShadow(cascadedShadowSampler3,
			lightRadiusUV, lightZNear, lightZFar,
			uv, z, dz_duv, zEye);
	}

	return 1.0 - shadow * (1.0 - ambient);
}

const bool debug_layer = false;

vec4 calcCSM(vec3 viewPos, vec3 worldPos)
{
	vec4 outColor;
	uint cascaded = calcCSMIndex(viewPos);
	float shadow = calcCSMShadow(cascaded, worldPos);
	if(debug_layer)
	{
		outColor = shadow * blendCSMColor(cascaded, vec4(1.0f));
	}
	else
	{
		outColor = vec4(shadow);
	}
	return outColor;
}

#endif