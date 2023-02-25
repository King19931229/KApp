#ifndef PUBLIC_H
#define PUBLIC_H

// TODO C++ Generate information

// Semantic
#define POSITION 0
#define NORMAL 1
#define TEXCOORD0 2
#define TEXCOORD1 3

#define DIFFUSE 4
#define SPECULAR 5

#define TANGENT 6
#define BINORMAL 7

#define BLEND_WEIGHTS 8
#define BLEND_INDICES 9

#define GUI_POS 10
#define GUI_UV 11
#define GUI_COLOR 12

#define SCREENQAUD_POS 13

#define INSTANCE_ROW_0 14
#define INSTANCE_ROW_1 15
#define INSTANCE_ROW_2 16

#define INSTANCE_PREV_ROW_0 17
#define INSTANCE_PREV_ROW_1 18
#define INSTANCE_PREV_ROW_2 19

#define TERRAIN_POS 20

// Binding
#define BINDING_CAMERA 0
#define BINDING_SHADOW 1
#define BINDING_DYNAMIC_CASCADED_SHADOW 2
#define BINDING_STATIC_CASCADED_SHADOW 3
#define BINDING_VOXEL 4
#define BINDING_VOXEL_CLIPMAP 5
#define BINDING_GLOBAL 6

#define BINDING_OBJECT 7
#define BINDING_SHADING 8

#define BINDING_POSITION_NORMAL_UV 9
#define BINDING_DIFFUSE_SPECULAR 10
#define BINDING_TANGENT_BINORMAL 11
#define BINDING_MESHLET_DESC 12
#define BINDING_MESHLET_PRIM 13

#define BINDING_TEXTURE0 14
#define BINDING_TEXTURE1 15
#define BINDING_TEXTURE2 16
#define BINDING_TEXTURE3 17
#define BINDING_TEXTURE4 18
#define BINDING_TEXTURE5 19
#define BINDING_TEXTURE6 20
#define BINDING_TEXTURE7 21
#define BINDING_TEXTURE8 22
#define BINDING_TEXTURE9 23
#define BINDING_TEXTURE10 24
#define BINDING_TEXTURE11 25
#define BINDING_TEXTURE12 26
#define BINDING_TEXTURE13 27
#define BINDING_TEXTURE14 28
#define BINDING_TEXTURE15 29
#define BINDING_TEXTURE16 30
#define BINDING_TEXTURE17 31
#define BINDING_TEXTURE18 32
#define BINDING_TEXTURE19 33
#define BINDING_TEXTURE20 34
#define BINDING_TEXTURE21 35
#define BINDING_TEXTURE22 36
#define BINDING_TEXTURE23 37
#define BINDING_TEXTURE24 38
#define BINDING_TEXTURE25 39
#define BINDING_TEXTURE26 40
#define BINDING_TEXTURE27 40
#define BINDING_TEXTURE28 42
#define BINDING_TEXTURE29 43
#define BINDING_TEXTURE30 44
#define BINDING_TEXTURE31 45
#define BINDING_TEXTURE32 46
#define BINDING_TEXTURE33 47
#define BINDING_TEXTURE34 48
#define BINDING_TEXTURE35 49

#define BINDING_SM BINDING_TEXTURE0

#define BINDING_STATIC_CSM0 BINDING_TEXTURE1
#define BINDING_STATIC_CSM1 BINDING_TEXTURE2
#define BINDING_STATIC_CSM2 BINDING_TEXTURE3
#define BINDING_STATIC_CSM3 BINDING_TEXTURE4

#define BINDING_DYNAMIC_CSM0 BINDING_TEXTURE5
#define BINDING_DYNAMIC_CSM1 BINDING_TEXTURE6
#define BINDING_DYNAMIC_CSM2 BINDING_TEXTURE7
#define BINDING_DYNAMIC_CSM3 BINDING_TEXTURE8

#define BINDING_DIFFUSE_IRRADIANCE BINDING_TEXTURE9
#define BINDING_SPECULAR_IRRADIANCE BINDING_TEXTURE10
#define BINDING_INTEGRATE_BRDF BINDING_TEXTURE11

#define BINDING_MATERIAL0 BINDING_TEXTURE12
#define BINDING_MATERIAL1 BINDING_TEXTURE13
#define BINDING_MATERIAL2 BINDING_TEXTURE14
#define BINDING_MATERIAL3 BINDING_TEXTURE15
#define BINDING_MATERIAL4 BINDING_TEXTURE16

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
	mat4 viewproj[18];
	mat4 viewproj_inv[18];
	// Atmost 3 volume per clipmap
	vec4 update_region_min[18];
	vec4 update_region_max[18];
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
} global;

const float PI = 3.14159265f;
const float HALF_PI = 1.57079f;

struct MaterialPixelParameters
{
	vec3 position;
	vec3 normal;
	vec2 motion;
	vec3 baseColor;
	float metal;
	float roughness;
};

#endif