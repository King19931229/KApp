#ifndef _VOLUMETRIC_FOG_PUBLIC_H_
#define _VOLUMETRIC_FOG_PUBLIC_H_

#include "public.h"
#include "common.h"

#define GROUP_SIZE 8
#define BINDING_VOXEL_PREV BINDING_TEXTURE0
#define BINDING_VOXEL_CURR BINDING_TEXTURE1
#define BINDING_VOXEL_RESULT BINDING_TEXTURE2
#define BINDING_GBUFFER_RT0 BINDING_TEXTURE3

const float EPSILON = 1e-4;

#define LINEAR_DEPTH_SPLIT 0

float Exp01DepthToLinear01Depth(float z, float n, float f)
{
	float z_buffer_params_y = f / n;
	float z_buffer_params_x = 1.0f - z_buffer_params_y;

	return 1.0f / (z_buffer_params_x * z + z_buffer_params_y);
}

float Linear01DepthToExp01Depth(float z, float n, float f)
{
	float z_buffer_params_y = f / n;
	float z_buffer_params_x = 1.0f - z_buffer_params_y;

	return (1.0f / z - z_buffer_params_y) / z_buffer_params_x;
}

vec3 WorldToNDC(vec3 world_pos, mat4 viewProj)
{
	vec4 p = viewProj * vec4(world_pos, 1.0f);
	return p.xyz / p.w;
}

vec3 NDCToUV(vec3 ndc, mat4 proj, float n, float f, float grid_size_z)
{
	vec3 uv;

	uv.x = ndc.x * 0.5f + 0.5f;
	uv.y = ndc.y * 0.5f + 0.5f;
	//uv.z = NonLinearDepthToLinearDepth(proj, ndc.z);
	uv.z = Exp01DepthToLinear01Depth(ndc.z, n, f);
	float view_z = uv.z * f;
#if LINEAR_DEPTH_SPLIT
	uv.z = (view_z - n) / float(f - n);
#else
	// Exponential View-Z
	vec2 params = vec2(float(grid_size_z) / log2(f / n), -(float(grid_size_z) * log2(n) / log2(f / n)));	
	uv.z = (max(log2(view_z) * params.x + params.y, 0.0f)) / grid_size_z;
#endif

	return uv;
}

vec3 WorldToUV(vec3 world_pos, mat4 viewProj, mat4 proj, float n, float f, float grid_size_z)
{
	vec3 ndc = WorldToNDC(world_pos, viewProj);
	return NDCToUV(ndc, proj, n, f, grid_size_z);
}

vec3 UVToNDC(vec3 uv, mat4 proj, float n, float f)
{
	vec3 ndc;
	ndc.x = 2.0f * uv.x - 1.0f;
	ndc.y = 2.0f * uv.y - 1.0f;
	//ndc.z = LinearDepthToNonLinearDepth(proj, uv.z);
	ndc.z = Linear01DepthToExp01Depth(uv.z, n, f);
	return ndc;
}

vec3 NDCToWorld(vec3 ndc, mat4 invViewProj)
{
	vec4 p = invViewProj * vec4(ndc, 1.0f);
	return p.xyz / p.w;
}

vec3 IDToUV(ivec3 id, float n, float f, float grid_size_x, float grid_size_y, float grid_size_z)
{
#if LINEAR_DEPTH_SPLIT
	float view_z = n + (f - n) * (float(id.z) + 0.5f) / float(grid_size_z);
#else
	// Exponential View-Z
	float view_z = n * pow(f / n, (float(id.z) + 0.5f) / float(grid_size_z));
#endif

	return vec3((float(id.x) + 0.5f) / float(grid_size_x),
				(float(id.y) + 0.5f) / float(grid_size_y),
				view_z / f);
}

vec3 IDToUVWithJitter(ivec3 id, float n, float f, float jitter, float grid_size_x, float grid_size_y, float grid_size_z)
{
#if LINEAR_DEPTH_SPLIT
	float view_z = n + (f - n) * (float(id.z) + 0.5f + jitter) / float(grid_size_z);
#else
	// Exponential View-Z
	float view_z = n * pow(f / n, (float(id.z) + 0.5f + jitter) / float(grid_size_z));
#endif

	return vec3((float(id.x) + 0.5f) / float(grid_size_x),
				(float(id.y) + 0.5f) / float(grid_size_y),
				view_z / f);
}

vec3 IDToWorldWithoutJitter(ivec3 id, float n, float f, mat4 proj, mat4 invViewProj, float grid_size_x, float grid_size_y, float grid_size_z)
{
	vec3 uv = IDToUV(id, n, f, grid_size_x, grid_size_y, grid_size_z);
	vec3 ndc = UVToNDC(uv, proj, n, f);
	return NDCToWorld(ndc, invViewProj);
}

vec3 IDToWorldWithJitter(ivec3 id, float n, float f, float jitter, mat4 proj, mat4 invViewProj, float grid_size_x, float grid_size_y, float grid_size_z)
{
	vec3 uv = IDToUVWithJitter(id, n, f, jitter, grid_size_x, grid_size_y, grid_size_z);
	vec3 ndc = UVToNDC(uv, proj, n, f);
	return NDCToWorld(ndc, invViewProj);
}

float SliceDistance(int z, float n, float f, float grid_size_z)
{
#if LINEAR_DEPTH_SPLIT
	return n + (f - n) * (float(z) + 0.5f) / float(grid_size_z);
#else
	return n * pow(f / n, (float(z) + 0.5f) / float(grid_size_z));
#endif
}

float SliceThickness(int z, float n, float f, float grid_size_z)
{
	return abs(SliceDistance(z + 1, n, f, grid_size_z) - SliceDistance(z, n, f, grid_size_z));
}

// https://gist.github.com/Fewes/59d2c831672040452aa77da6eaab2234
vec4 TextureTricubic(sampler3D tex, vec3 coord)
{
	// Shift the coordinate from [0,1] to [-0.5, texture_size-0.5]
	vec3 texture_size = vec3(textureSize(tex, 0));
	vec3 coord_grid = coord * texture_size - 0.5;
	vec3 index = floor(coord_grid);
	vec3 fraction = coord_grid - index;
	vec3 one_frac = 1.0 - fraction;

	vec3 w0 = 1.0/6.0 * one_frac*one_frac*one_frac;
	vec3 w1 = 2.0/3.0 - 0.5 * fraction*fraction*(2.0-fraction);
	vec3 w2 = 2.0/3.0 - 0.5 * one_frac*one_frac*(2.0-one_frac);
	vec3 w3 = 1.0/6.0 * fraction*fraction*fraction;

	vec3 g0 = w0 + w1;
	vec3 g1 = w2 + w3;
	vec3 mult = 1.0 / texture_size;
	vec3 h0 = mult * ((w1 / g0) - 0.5 + index); //h0 = w1/g0 - 1, move from [-0.5, texture_size-0.5] to [0,1]
	vec3 h1 = mult * ((w3 / g1) + 1.5 + index); //h1 = w3/g1 + 1, move from [-0.5, texture_size-0.5] to [0,1]

	// Fetch the eight linear interpolations
	// Weighting and fetching is interleaved for performance and stability reasons
	vec4 tex000 = texture(tex, h0, 0.0f);
	vec4 tex100 = texture(tex, vec3(h1.x, h0.y, h0.z), 0.0f);
	tex000 = mix(tex100, tex000, g0.x); // Weigh along the x-direction

	vec4 tex010 = texture(tex, vec3(h0.x, h1.y, h0.z), 0.0f);
	vec4 tex110 = texture(tex, vec3(h1.x, h1.y, h0.z), 0.0f);
	tex010 = mix(tex110, tex010, g0.x); // Weigh along the x-direction
	tex000 = mix(tex010, tex000, g0.y); // Weigh along the y-direction

	vec4 tex001 = texture(tex, vec3(h0.x, h0.y, h1.z), 0.0f);
	vec4 tex101 = texture(tex, vec3(h1.x, h0.y, h1.z), 0.0f);
	tex001 = mix(tex101, tex001, g0.x); // Weigh along the x-direction

	vec4 tex011 = texture(tex, vec3(h0.x, h1.y, h1.z), 0.0f);
	vec4 tex111 = texture(tex, vec3(h1), 0.0f);
	tex011 = mix(tex111, tex011, g0.x); // Weigh along the x-direction
	tex001 = mix(tex011, tex001, g0.y); // Weigh along the y-direction

	return mix(tex001, tex000, g0.z); // Weigh along the z-direction
}

layout(binding = BINDING_OBJECT)
uniform Object
{
	mat4 view;
	mat4 proj;
	mat4 viewProj;
	mat4 invViewProj;

	mat4 prevView;
	mat4 prevProj;
	mat4 prevViewProj;
	mat4 prevInvViewProj;

	vec4 nearFarGridZ;
	vec4 anisotropyDensityScatteringAbsorption;
	vec4 cameraPos;

	uint frameNum;
} object;

#endif