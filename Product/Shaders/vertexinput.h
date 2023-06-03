#ifndef VERTEX_INPUT_H
#define VERTEX_INPUT_H

#ifndef INSTANCE_INPUT
#	define INSTANCE_INPUT 0
#endif

#ifndef MESHLET_INPUT
#	define MESHLET_INPUT 0
#endif

#include "public.h"

#if MESHLET_INPUT
/*
struct VertexPack
{
	vec3 position;
	vec3 normal;
	vec2 texcoord0;
};
*/
layout(binding = BINDING_POSITION_NORMAL_UV, scalar) readonly buffer VertexPackBuffer { float meshletVertex[]; };

#if TANGENT_BINORMAL_INPUT
/*
struct TangentBinormalPack
{
	vec3 tangent;
	vec3 binormal;
};
*/
layout(binding = BINDING_TANGENT_BINORMAL, scalar) readonly buffer TangentBinormalPackBuffer { float meshletTangentBinormal[]; };
#endif

layout(std430, binding = BINDING_MESHLET_DESC) readonly buffer MeshletDescBuffer { uvec4 meshletDescs[]; };
layout(std430, binding = BINDING_MESHLET_PRIM) readonly buffer PrimIndexBuffer1 { uint primIndices1[]; };
layout(std430, binding = BINDING_MESHLET_PRIM) readonly buffer PrimIndexBuffer2 { uvec2 primIndices2[]; };

#else

layout(location = POSITION) in vec3 position;
layout(location = NORMAL) in vec3 normal;
layout(location = TEXCOORD0) in vec2 texcoord0;

#if UV2_INPUT
layout(location = TEXCOORD1) in vec2 texcoord1;
#endif

#if TANGENT_BINORMAL_INPUT
layout(location = TANGENT) in vec3 tangent;
layout(location = BINORMAL) in vec3 binormal;
#endif

#if VERTEX_COLOR_INPUT0
layout(location = COLOR0) in vec3 color0;
#endif

#if VERTEX_COLOR_INPUT1
layout(location = COLOR1) in vec3 color1;
#endif

#if VERTEX_COLOR_INPUT2
layout(location = COLOR2) in vec3 color2;
#endif

#if VERTEX_COLOR_INPUT3
layout(location = COLOR3) in vec3 color3;
#endif

#if VERTEX_COLOR_INPUT4
layout(location = COLOR4) in vec3 color4;
#endif

#if VERTEX_COLOR_INPUT5
layout(location = COLOR5) in vec3 color5;
#endif

#if INSTANCE_INPUT

layout(location = INSTANCE_ROW_0) in vec4 world_row0;
layout(location = INSTANCE_ROW_1) in vec4 world_row1;
layout(location = INSTANCE_ROW_2) in vec4 world_row2;
#define WORLD_MATRIX (transpose(mat4(world_row0, world_row1, world_row2, vec4(0, 0, 0, 1))))

layout(location = INSTANCE_PREV_ROW_0) in vec4 prev_world_row0;
layout(location = INSTANCE_PREV_ROW_1) in vec4 prev_world_row1;
layout(location = INSTANCE_PREV_ROW_2) in vec4 prev_world_row2;
#define PREV_WORLD_MATRIX (transpose(mat4(prev_world_row0, prev_world_row1, prev_world_row2, vec4(0, 0, 0, 1))))

#else

#define WORLD_MATRIX (object.model)
#define PREV_WORLD_MATRIX (object.prev_model)

#endif // INSTANCE_INPUT

#endif // MESHLET_INPUT

#endif // VERTEX_INPUT_H