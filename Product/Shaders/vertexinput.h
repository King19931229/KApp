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

struct VertexPack
{
	vec3 position;
	vec3 normal;
	vec2 texcoord0;
};

layout(binding = BINDING_POSITION_NORMAL_UV, scalar) readonly buffer VertexPackBuffer { VertexPack meshletVertex[]; };

#if DIFFUSE_SPECULAR_INPUT
struct DiffuseSpecularPack
{
	vec3 diffuse;
	vec3 specular;
};
layout(binding = BINDING_DIFFUSE_SPECULAR, scalar) readonly buffer DiffuseSpecularPackBuffer { DiffuseSpecularPack meshletDiffuseSpecular[]; };
#endif

#if TANGENT_BINORMAL_INPUT
struct TangentBinormalPack
{
	vec3 tangent;
	vec3 binormal;
};
layout(binding = BINDING_TANGENT_BINORMAL, scalar) readonly buffer TangentBinormalPackBuffer { TangentBinormalPack meshletTangentBinormal[]; };
#endif

layout(std430, binding = BINDING_MESHLET_DESC) buffer MeshletDescBuffer { uvec4 meshletDescs[]; };
layout(std430, binding = BINDING_MESHLET_PRIM) buffer PrimIndexBuffer1 { uint primIndices1[]; };
layout(std430, binding = BINDING_MESHLET_PRIM) buffer PrimIndexBuffer2 { uvec2 primIndices2[]; };

#else

layout(location = POSITION) in vec3 position;
layout(location = NORMAL) in vec3 normal;
layout(location = TEXCOORD0) in vec2 texcoord0;

#if DIFFUSE_SPECULAR_INPUT
layout(location = DIFFUSE) in vec3 diffuse;
layout(location = SPECULAR) in vec3 specular;
#endif

#if TANGENT_BINORMAL_INPUT
layout(location = TANGENT) in vec3 tangent;
layout(location = BINORMAL) in vec3 binormal;
#endif

#if INSTANCE_INPUT

layout(location = INSTANCE_ROW_0) in vec4 world_row0;
layout(location = INSTANCE_ROW_1) in vec4 world_row1;
layout(location = INSTANCE_ROW_2) in vec4 world_row2;
#define WORLD_MATRIX transpose(mat4(world_row0, world_row1, world_row2, vec4(0, 0, 0, 1)));

#else

#define WORLD_MATRIX object.model

#endif // INSTANCE_INPUT

#endif // MESHLET_INPUT

#endif // VERTEX_INPUT_H