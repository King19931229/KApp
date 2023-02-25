#ifndef _GBUFFER_H_
#define _GBUFFER_H_

#define GBUFFER_IMAGE0_FORMAT rgba16f
#define GBUFFER_IMAGE1_FORMAT rgba16f
#define GBUFFER_IMAGE2_FORMAT rgba8
#define GBUFFER_IMAGE3_FORMAT rgba8
#define AO_IMAGE_FORMAT r8

#include "common.h"

vec3 DecodeNormal(vec4 gbuffer0Data)
{
	return gbuffer0Data.xyz;
}

vec3 DecodeNormalViewSpace(vec4 gbuffer0Data)
{
	vec4 normal = vec4(gbuffer0Data.xyz, 0.0);
	normal = camera.view * normal;
	return normal.xyz;
}

float DecodeDepth(vec4 gbuffer0Data)
{
	return gbuffer0Data.w;
}

vec3 DecodePosition(vec4 gbuffer0Data, vec2 screenUV)
{
	float depth = LinearDepthToNonLinearDepth(camera.proj, gbuffer0Data.w);
	vec3 ndc = vec3(2.0 * screenUV - vec2(1.0), depth);
	vec4 worldPosH = camera.viewInv * camera.projInv * vec4(ndc, 1.0);
	return worldPosH.xyz / worldPosH.w;
}

vec3 DecodePositionViewSpace(vec4 gbuffer0Data, vec2 screenUV)
{
	float depth = LinearDepthToNonLinearDepth(camera.proj, gbuffer0Data.w);
	vec3 ndc = vec3(2.0 * screenUV - vec2(1.0), depth);
	vec4 viewPosH = camera.projInv * vec4(ndc, 1.0);
	return viewPosH.xyz / viewPosH.w;
}

vec2 DecodeMotion(vec4 gbuffer1Data)
{
	return gbuffer1Data.xy;
}

vec3 DecodeBaseColor(vec4 gbuffer2Data)
{
	return gbuffer2Data.xyz;
}

float DecodeRoughness(vec4 gbuffer3Data)
{
	return gbuffer3Data.x;
}

float DecodeMetal(vec4 gbuffer3Data)
{
	return gbuffer3Data.y;
}

#endif