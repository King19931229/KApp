#ifndef PUBLIC_H
#define PUBLIC_H

// TODO C++ Generate info

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

// Binding
#define BINDING_CAMERA 0
#define BINDING_SHADOW 1
#define BINDING_CASCADED_SHADOW 2
#define BINDING_GLOBAL 3

#define BINDING_OBJECT 4
#define BINDING_VERTEX_SHADING 5
#define BINDING_FRAGMENT_SHADING 6

#define BINDING_TEXTURE0 7
#define BINDING_TEXTURE1 8
#define BINDING_TEXTURE2 9
#define BINDING_TEXTURE3 10
#define BINDING_TEXTURE4 11
#define BINDING_TEXTURE5 12
#define BINDING_TEXTURE6 13
#define BINDING_TEXTURE7 14
#define BINDING_TEXTURE8 15
#define BINDING_TEXTURE9 16
#define BINDING_TEXTURE10 17
#define BINDING_TEXTURE11 18

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

layout(binding = BINDING_CASCADED_SHADOW)
uniform CascadedShadowInfo
{
	mat4 light_view[4];
	mat4 light_view_proj[4];
	vec4 lightInfo[4];
	vec4 frustum;
	uint cascaded;
}cascaded_shadow;

layout(binding = BINDING_GLOBAL)
uniform GlobalInfo
{
	vec4 sunLightDir;
}global;

#include "pbr.h"

#endif