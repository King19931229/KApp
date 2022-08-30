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
#define BINDING_VERTEX_SHADING 8
#define BINDING_FRAGMENT_SHADING 9

#define BINDING_POSITION_NORMAL_UV 10
#define BINDING_DIFFUSE_SPECULAR 11
#define BINDING_TANGENT_BINORMAL 12
#define BINDING_MESHLET_DESC 13
#define BINDING_MESHLET_PRIM 14

#define BINDING_TEXTURE0 15
#define BINDING_TEXTURE1 16
#define BINDING_TEXTURE2 17
#define BINDING_TEXTURE3 18
#define BINDING_TEXTURE4 19
#define BINDING_TEXTURE5 20
#define BINDING_TEXTURE6 21
#define BINDING_TEXTURE7 22
#define BINDING_TEXTURE8 23
#define BINDING_TEXTURE9 24
#define BINDING_TEXTURE10 25
#define BINDING_TEXTURE11 26
#define BINDING_TEXTURE12 27
#define BINDING_TEXTURE13 28
#define BINDING_TEXTURE14 29
#define BINDING_TEXTURE15 30
#define BINDING_TEXTURE16 31
#define BINDING_TEXTURE17 32
#define BINDING_TEXTURE18 33
#define BINDING_TEXTURE19 34
#define BINDING_TEXTURE20 35
#define BINDING_TEXTURE21 36
#define BINDING_TEXTURE22 37
#define BINDING_TEXTURE23 38
#define BINDING_TEXTURE24 39
#define BINDING_TEXTURE25 40
#define BINDING_TEXTURE26 41

#define BINDING_DIFFUSE BINDING_TEXTURE0
#define BINDING_SPECULAR BINDING_TEXTURE1
#define BINDING_NORMAL BINDING_TEXTURE2

#define BINDING_MATERIAL0 BINDING_TEXTURE0
#define BINDING_MATERIAL1 BINDING_TEXTURE1
#define BINDING_MATERIAL2 BINDING_TEXTURE2
#define BINDING_MATERIAL3 BINDING_TEXTURE3

#define BINDING_SM BINDING_TEXTURE4
#define BINDING_CSM0 BINDING_TEXTURE5
#define BINDING_CSM1 BINDING_TEXTURE6
#define BINDING_CSM2 BINDING_TEXTURE7
#define BINDING_CSM3 BINDING_TEXTURE8

#define BINDING_DIFFUSE_IRRADIANCE BINDING_TEXTURE9
#define BINDING_SPECULAR_IRRADIANCE BINDING_TEXTURE10
#define BINDING_INTEGRATE_BRDF BINDING_TEXTURE11

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
}camera;

layout(binding = BINDING_SHADOW)
uniform ShadowInfo
{
	mat4 light_view;
	mat4 light_proj;
	vec2 near_far;
}shadow;

layout(binding = BINDING_DYNAMIC_CASCADED_SHADOW)
uniform DynamicCascadedShadowInfo
{
	mat4 light_view[4];
	mat4 light_view_proj[4];
	vec4 lightInfo[4];
	vec4 frustum;
	uint cascaded;
}dynamic_cascaded;

layout(binding = BINDING_STATIC_CASCADED_SHADOW)
uniform StaticCascadedShadowInfo
{
	mat4 light_view[4];
	mat4 light_view_proj[4];
	vec4 lightInfo[4];
	vec4 frustum;
	uint cascaded;
}static_cascaded;

layout(binding = BINDING_VOXEL)
uniform VoxelInfo
{
	mat4 viewproj[3];
	mat4 viewproj_inv[3];
	vec4 sunlight;
	// scale: 1.0 / volumeSize
	vec4 minpoint_scale;
	vec4 maxpoint_scale;
	// volumeDimension:1 storeVisibility:1 normalWeightedLambert:1 checkBoundaries:1
	uvec4 miscs;
	// voxelSize:1 volumeSize:1 exponents:2
	vec4 miscs2;
	// lightBleedingReduction:1 traceShadowHit:1 maxTracingDistanceGlobal:1
	vec4 miscs3;
}voxel;

layout(binding = BINDING_VOXEL_CLIPMAP)
uniform VoxelClipmapInfo
{
	mat4 viewproj[6][3];
	mat4 viewproj_inv[6][3];
	vec4 reigon_min_and_voxelsize[6];
	vec4 reigon_max_and_extent[6];
	// volumeDimension:1 borderSize:1 storeVisibility:1
	uvec4 miscs;
}voxel_clipmap;

layout(binding = BINDING_GLOBAL)
uniform GlobalInfo
{
	vec4 sunLightDir;
}global;

const float PI = 3.14159265f;
const float HALF_PI = 1.57079f;

#endif