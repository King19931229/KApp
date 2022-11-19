#ifndef _GBUFFER_H_
#define _GBUFFER_H_

#define GBUFFER_IMAGE0_FORMAT rgba16f
#define GBUFFER_IMAGE1_FORMAT rgba16f
#define GBUFFER_IMAGE2_FORMAT rgba8
#define GBUFFER_IMAGE3_FORMAT rgba8

vec3 DecodeNormal(vec4 gbuffer0Data)
{
	return gbuffer0Data.xyz;
}

float DecodeDepth(vec4 gbuffer0Data)
{
	return gbuffer0Data.w;
}

vec3 DecodePosition(vec4 gbuffer0Data, vec2 screenUV)
{
	float near = camera.proj[3][2] / camera.proj[2][2];
	float far = -camera.proj[3][2] / (camera.proj[2][3] - camera.proj[2][2]);

	float z = -(near + gbuffer0Data.w * (far - near));
	float depth = (z * camera.proj[2][2] + camera.proj[3][2]) / (z * camera.proj[2][3]);

	vec3 ndc = vec3(2.0 * screenUV - vec2(1.0), depth);
	vec4 worldPosH = camera.viewInv * camera.projInv * vec4(ndc, 1.0);

	return worldPosH.xyz / worldPosH.w;
}

vec2 DecodeMotion(vec4 gbuffer1Data)
{
	return gbuffer1Data.xy; // 2.0 * gbuffer1Data.xy - vec2(1.0);
}

vec3 DecodeBaseColor(vec4 gbuffer2Data)
{
	return gbuffer2Data.xyz;
}

vec3 DecodeSpecularColor(vec4 gbuffer3Data)
{
	return gbuffer3Data.xyz;
}

#endif