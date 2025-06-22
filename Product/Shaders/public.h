#ifndef PUBLIC_H
#define PUBLIC_H

/* Shader compiler will generate this*/
#include "binding_generate_code.h"

#define CUBEMAP_UVW(uvw) vec3(-uvw.x, -uvw.y, -uvw.z)

#extension GL_EXT_scalar_block_layout : enable

layout(binding = BINDING_CAMERA)
uniform CameraInfo
{
	mat4 view;
	mat4 proj;
	mat4 viewInv;
	mat4 projInv;
	mat4 viewProj;
	mat4 prevViewProj;
	// near, far, fov, aspect
	vec4 parameters;
	vec4 frustumPlanes[6];
} camera;

layout(binding = BINDING_SHADOW)
uniform ShadowInfo
{
	mat4 light_view;
	mat4 light_proj;
	vec2 near_far;
} shadow;

layout(binding = BINDING_DYNAMIC_CASCADED_SHADOW)
uniform DynamicCascadedShadowInfo
{
	mat4 light_view[4];
	mat4 light_view_proj[4];
	vec4 lightInfo[4];
	vec4 frustumPlanes[24];
	vec4 splitDistance;
	vec4 center;
	uint cascaded;
} dynamic_cascaded;

layout(binding = BINDING_STATIC_CASCADED_SHADOW)
uniform StaticCascadedShadowInfo
{
	mat4 light_view[4];
	mat4 light_view_proj[4];
	vec4 lightInfo[4];
	vec4 frustumPlanes[24];
	vec4 area;
	vec4 center;
	uint cascaded;
} static_cascaded;

layout(binding = BINDING_VOXEL)
uniform VoxelInfo
{
	mat4 viewproj[3];
	mat4 viewproj_inv[3];
	// scale: 1.0 / volumeSize
	vec4 minpoint_scale;
	vec4 maxpoint_scale;
	// volumeDimension:1 storeVisibility:1 normalWeightedLambert:1 checkBoundaries:1
	uvec4 miscs;
	// voxelSize:1 volumeSize:1 exponents:2
	vec4 miscs2;
	// lightBleedingReduction:1 traceShadowHit:1 maxTracingDistanceGlobal:1
	vec4 miscs3;
} voxel;

layout(binding = BINDING_VOXEL_CLIPMAP)
uniform VoxelClipmapInfo
{
	// 3 axis per clipmap
	mat4 viewproj[27];
	mat4 viewproj_inv[27];
	// Atmost 3 volume per clipmap
	vec4 update_region_min[27];
	vec4 update_region_max[27];
	// Atmost 9 clipmap
	vec4 region_min_and_voxelsize[9];
	vec4 region_max_and_extent[9];
	// volumeDimension:1 borderSize:1 storeVisibility:1 normalWeightedLambert:1
	uvec4 miscs;
	// levelCount:1 checkBoundaries:1
	uvec4 miscs2;
	// traceShadowHit:1 maxTracingDistanceGlobal:1 occlusionDecay:1 downsampleTransitionRegionSize:1
	vec4 miscs3;
	vec4 miscs4;
} voxel_clipmap;

layout(binding = BINDING_GLOBAL)
uniform GlobalInfo
{
	vec4 sunLightDirAndMaxPBRLod;
	uvec4 miscs;
} global;

layout(binding = BINDING_VIRTUAL_TEXTURE_CONSTANT)
uniform VirtualTextureConstantInfo
{
	uvec4 description;
	uvec4 description2;
} virtual_texture_constant;

const float PI = 3.14159265f;
const float HALF_PI = 1.57079f;

struct MaterialPixelParameters
{
	vec3 position;
	vec3 normal;
	vec2 motion;
	vec3 baseColor;
	vec3 emissive;
	float metal;
	float roughness;
	float ao;
	float opacity;
};
#endif