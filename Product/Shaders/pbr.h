#ifndef PBR_H
#define PBR_H

#define MEDIUMP_FLT_MAX	65504.0
#define MEDIUMP_FLT_MIN 0.00006103515625

float DistributionGGX(vec3 N, vec3 H, float a)
{
	float a2		= max(a * a, MEDIUMP_FLT_MIN);
	float NdotH		= max(dot(N, H), MEDIUMP_FLT_MIN);
	float NdotH2	= NdotH * NdotH;

	float num		= a2;
	float denom		= (NdotH2 * (a2 - 1.0) + 1.0);
	denom			= PI * denom * denom;

	return num / denom;
}

float GeometrySmithGGX(vec3 N, vec3 V, float a)
{
	float a2		= max(a * a, MEDIUMP_FLT_MIN);
	float NdotV		= max(dot(N, V), MEDIUMP_FLT_MIN);
	float NdotV2	= NdotV * NdotV;

	float num		= 2.0 * NdotV;
	float denom		= NdotV + sqrt(NdotV2* (1 - a2) + a2);

	return num / denom;
}

float GeometrySmithGGXJoint(vec3 N, vec3 V, vec3 L, float roughness)
{
	float ggx2  = GeometrySmithGGX(N, V, roughness);
	float ggx1  = GeometrySmithGGX(N, L, roughness);
	return ggx1 * ggx2;
}

vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
	return F0 + (1.0 - F0) * pow(max(1.0 - cosTheta, 0.0), 5.0);
}

vec3 FresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
	return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(max(1.0 - cosTheta, 0.0), 5.0);
}

float RadicalInverse_VdC(uint bits) 
{
	bits = (bits << 16u) | (bits >> 16u);
	bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
	bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
	bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
	bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
	return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

vec2 Hammersley(uint i, uint N)
{
	return vec2(float(i)/float(N), RadicalInverse_VdC(i));
}

vec4 ImportanceSampleGGX(vec2 Xi, vec3 N, float roughness)
{
	float a = roughness * roughness;

	float phi = 2.0 * PI * Xi.x;
	float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a * a - 1.0) * Xi.y));
	float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

	// from spherical coordinates to cartesian coordinates
	vec3 H;
	H.x = cos(phi) * sinTheta;
	H.y = sin(phi) * sinTheta;
	H.z = cosTheta;

	// from tangent-space vector to world-space sample vector
	vec3 up        = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
	vec3 tangent   = normalize(cross(up, N));
	vec3 bitangent = cross(N, tangent);

	vec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;

	float a2 = a * a;
	float d = max((cosTheta * a2 - cosTheta) * cosTheta + 1, MEDIUMP_FLT_MIN);
	float D = a2 / (PI * d * d);
	float pdf = D * cosTheta;

	return vec4(normalize(sampleVec), pdf);
}

// https://www.shadertoy.com/view/MlXXW4
vec4 TextureCubeFixed(const in samplerCube tex, in vec3 P)
{
	vec3 absP = abs(P);
	vec3 mask = step(absP.xyz, max(absP.yzx, absP.zxy));
	vec2 cubeResolution = vec2(textureSize(tex, 0));
	vec3 scale = vec3(cubeResolution.x / (cubeResolution.x - 1.0), cubeResolution.y / (cubeResolution.y - 1.0), 1.0);
	P *= mix(scale, vec3(1.0), mask);
	return texture(tex, P);
}

#endif