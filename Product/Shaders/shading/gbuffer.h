#ifndef _GBUFFER_H_
#define _GBUFFER_H_

#define GBUFFER_IMAGE0_FORMAT rgba16f
#define GBUFFER_IMAGE1_FORMAT rg16f
#define GBUFFER_IMAGE2_FORMAT rgba8
#define GBUFFER_IMAGE3_FORMAT rg8
#define GBUFFER_IMAGE4_FORMAT rgba8
#define AO_IMAGE_FORMAT r8

#include "numerical.h"

vec3 DecodeNormal(vec4 gbuffer0Data)
{
	return gbuffer0Data.xyz;
}

// TODO No camera.XXX
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

float DecodeAO(vec4 gbuffer2Data)
{
	return gbuffer2Data.w;
}

float DecodeMetal(vec4 gbuffer3Data)
{
	return gbuffer3Data.x;
}

float DecodeRoughness(vec4 gbuffer3Data)
{
	return gbuffer3Data.y;
}

vec3 DecodeEmissive(vec4 gbuffer4Data)
{
	return gbuffer4Data.xyz;
}

struct GBufferEncodeData
{
	vec4 gbuffer0;
	vec4 gbuffer1;
	vec4 gbuffer2;
	vec4 gbuffer3;
	vec4 gbuffer4;
	vec2 uv;
};

struct GBufferDecodeData
{
	vec3 worldPos;
	vec3 worldNormal;
	vec2 motion;
	vec3 baseColor;
	vec3 emissive;
	float metal;
	float roughness;
	float ao;
};

GBufferEncodeData EncodeGBuffer(mat4 view, mat4 proj, vec3 pos, vec3 normal, vec2 motion, vec3 baseColor, vec3 emissive, float metal, float roughness, float ao)
{
	vec4 viewPos = view * vec4(pos, 1.0);
	float near = proj[3][2] / proj[2][2];
	float far = -proj[3][2] / (proj[2][3] - proj[2][2]);
	float depth = (-viewPos.z - near) / (far - near);

	GBufferEncodeData data;

	data.gbuffer0.xyz = normalize(normal);
	data.gbuffer0.w = depth;
	data.gbuffer1.xy = vec2(motion.x, motion.y);
	data.gbuffer2.xyz = baseColor;
	data.gbuffer2.w = ao;
	data.gbuffer3.x = metal;
	data.gbuffer3.y = roughness;
	data.gbuffer4.xyz = emissive;

	return data;
}

GBufferDecodeData DecodeGBufferData(GBufferEncodeData encode)
{
	GBufferDecodeData data;

	data.worldPos = DecodePosition(encode.gbuffer0, encode.uv);
	data.worldNormal = DecodeNormal(encode.gbuffer0);
	data.motion = DecodeMotion(encode.gbuffer1);
	data.baseColor = DecodeBaseColor(encode.gbuffer2);
	data.ao = DecodeAO(encode.gbuffer2);
	data.metal = DecodeMetal(encode.gbuffer3);
	data.roughness = DecodeRoughness(encode.gbuffer3);
	data.emissive = DecodeEmissive(encode.gbuffer4);

	return data;
}

#endif