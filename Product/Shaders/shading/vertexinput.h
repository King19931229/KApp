#ifndef VERTEX_INPUT_H
#define VERTEX_INPUT_H

#include "public.h"

#ifdef GPU_SCENE

#include "gpuscene/gpuscene_define.h"

layout(std430, binding = BINDING_POINT_NORMAL_UV) readonly buffer PointNormalUVPackBuffer { float PointNormalUVData[]; };

layout(std430, binding = BINDING_TANGENT_BINORMAL) readonly buffer TangentBinormalPackBuffer { float TangentBinormalData[]; };

layout(std430, binding = BINDING_BLEND_WEIGHTS_INDICES) readonly buffer BlendWeightsIndicesPackBuffer { float BlendWeightsIndicesData[]; };

layout(std430, binding = BINDING_UV2) readonly buffer UV2PackBuffer { float UV2Data[]; };

layout(std430, binding = BINDING_COLOR0) readonly buffer Color0PackBuffer { float Color0Data[]; };

layout(std430, binding = BINDING_COLOR1) readonly buffer Color1PackBuffer { float Color1Data[]; };

layout(std430, binding = BINDING_COLOR2) readonly buffer Color2PackBuffer { float Color2Data[]; };

layout(std430, binding = BINDING_COLOR3) readonly buffer Color3PackBuffer { float Color3Data[]; };

layout(std430, binding = BINDING_COLOR4) readonly buffer Color4PackBuffer { float Color4Data[]; };

layout(std430, binding = BINDING_COLOR5) readonly buffer Color5PackBuffer { float Color5Data[]; };

layout(std430, binding = BINDING_INDEX) readonly buffer IndexPackBuffer { uint IndexData[]; };

layout(std430, binding = BINDING_MESH_STATE) buffer MeshStateBuffer { MeshStateStruct MeshState[]; };
layout(std430, binding = BINDING_INSTANCE_DATA) readonly buffer InstanceDataPackBuffer { InstanceStruct InstanceData[]; };

layout(std430, binding = BINDING_MATERIAL_PARAMETER) readonly buffer MaterialParameterPackBuffer { float MaterialParameterData[]; };
layout(std430, binding = BINDING_MATERIAL_TEXTURE_BINDING) readonly buffer MaterialTextureBindingPackBuffer { MaterialTextureBindingStruct MaterialTextureBinding[]; };

layout(std430, binding = BINDING_DRAWING_GROUP) readonly buffer DrawingGruopPackBuffer { uint DrawingGruop[]; };

layout(std430, binding = BINDING_MEGA_SHADER_STATE) readonly buffer MegaShaderStateBuffer { MegaShaderStateStruct MegaShaderState[]; };

// Must match KGPUSceneDrawParameter
layout(binding = BINDING_OBJECT)
uniform Object_DYN_UNIFORM
{
	uint megaShaderIndex;
} gpuscene;

uint groupIndex = (MegaShaderState[gpuscene.megaShaderIndex].groupWriteOffset + gl_InstanceIndex) < MegaShaderState[gpuscene.megaShaderIndex].instanceCount ? (MegaShaderState[gpuscene.megaShaderIndex].groupWriteOffset + gl_InstanceIndex) : 0;
uint instanceIndex = DrawingGruop[groupIndex];

uint meshIndex = InstanceData[instanceIndex].miscs[1];
uint shaderIndex = InstanceData[instanceIndex].miscs[2];
uint darwIndex = InstanceData[instanceIndex].miscs[3];

mat4 worldMatrix = InstanceData[instanceIndex].transform;
mat4 prevWorldMatrix = InstanceData[instanceIndex].prevTransform;

uint indexCount = MeshState[meshIndex].miscs[0];
uint vertexCount = MeshState[meshIndex].miscs[1];
uint indexOffset = MeshState[meshIndex].miscs[2];

uint vertexIndex[10] =
{
	MeshState[meshIndex].miscs[3 + 0] + IndexData[indexOffset + gl_VertexIndex],
	MeshState[meshIndex].miscs[3 + 1] + IndexData[indexOffset + gl_VertexIndex],
	MeshState[meshIndex].miscs[3 + 2] + IndexData[indexOffset + gl_VertexIndex],
	MeshState[meshIndex].miscs[3 + 3] + IndexData[indexOffset + gl_VertexIndex],
	MeshState[meshIndex].miscs[3 + 4] + IndexData[indexOffset + gl_VertexIndex],
	MeshState[meshIndex].miscs[3 + 5] + IndexData[indexOffset + gl_VertexIndex],
	MeshState[meshIndex].miscs[3 + 6] + IndexData[indexOffset + gl_VertexIndex],
	MeshState[meshIndex].miscs[3 + 7] + IndexData[indexOffset + gl_VertexIndex],
	MeshState[meshIndex].miscs[3 + 8] + IndexData[indexOffset + gl_VertexIndex],
	MeshState[meshIndex].miscs[3 + 9] + IndexData[indexOffset + gl_VertexIndex]
};

vec3 position = gl_VertexIndex < indexCount
	? vec3(PointNormalUVData[vertexIndex[0] * 8],PointNormalUVData[vertexIndex[0] * 8 + 1],PointNormalUVData[vertexIndex[0] * 8 + 2])
	: vec3(0);

vec3 normal = gl_VertexIndex < indexCount
	? vec3(PointNormalUVData[vertexIndex[0] * 8 + 3],PointNormalUVData[vertexIndex[0] * 8 + 4],PointNormalUVData[vertexIndex[0] * 8 + 5])
	: vec3(0);

vec2 texcoord0 = gl_VertexIndex < indexCount
	? vec2(PointNormalUVData[vertexIndex[0] * 8 + 6],PointNormalUVData[vertexIndex[0] * 8 + 7])
	: vec2(0);

#if TANGENT_BINORMAL_INPUT
vec3 tangent = gl_VertexIndex < indexCount
	? vec3(TangentBinormalData[vertexIndex[1] * 6],TangentBinormalData[vertexIndex[1] * 6 + 1],TangentBinormalData[vertexIndex[1] * 6 + 2])
	: vec3(0);

vec3 binormal = gl_VertexIndex < indexCount
	? vec3(TangentBinormalData[vertexIndex[1] * 6 + 3],TangentBinormalData[vertexIndex[1] * 6 + 4],TangentBinormalData[vertexIndex[1] * 6 + 5])
	: vec3(0);
#endif

#if BLEND_WEIGHT_INPUT

#endif

#if UV2_INPUT
vec2 texcoord1 = gl_VertexIndex < indexCount
	? vec2(UV2Data[vertexIndex[3] * 2], UV2Data[vertexIndex[3] * 2 + 1])
	: vec2(0);
#endif

#if VERTEX_COLOR_INPUT0
vec3 color0 = gl_VertexIndex < indexCount
	? vec3(Color0Data[vertexIndex[4] * 3],Color0Data[vertexIndex[4] * 3 + 1],Color0Data[vertexIndex[4] * 3 + 2])
	: vec3(0);
#endif
#if VERTEX_COLOR_INPUT1
vec3 color1 = gl_VertexIndex < indexCount
	? vec3(Color1Data[vertexIndex[5] * 3],Color1Data[vertexIndex[5] * 3 + 1],Color1Data[vertexIndex[5] * 3 + 2])
	: vec3(0);
#endif
#if VERTEX_COLOR_INPUT2
vec3 color2 = gl_VertexIndex < indexCount
	? vec3(Color2Data[vertexIndex[6] * 3],Color2Data[vertexIndex[6] * 3 + 1],Color2Data[vertexIndex[6] * 3 + 2])
	: vec3(0);
#endif
#if VERTEX_COLOR_INPUT3
vec3 color3 = gl_VertexIndex < indexCount
	? vec3(Color3Data[vertexIndex[7] * 3],Color3Data[vertexIndex[7] * 3 + 1],Color3Data[vertexIndex[7] * 3 + 2])
	: vec3(0);
#endif
#if VERTEX_COLOR_INPUT4
vec3 color4 = gl_VertexIndex < indexCount
	? vec3(Color4Data[vertexIndex[8] * 3],Color4Data[vertexIndex[8] * 3 + 1],Color4Data[vertexIndex[8] * 3 + 2])
	: vec3(0);
#endif
#if VERTEX_COLOR_INPUT5
vec3 color5 = gl_VertexIndex < indexCount
	? vec3(Color5Data[vertexIndex[9] * 3],Color5Data[vertexIndex[9] * 3 + 1],Color5Data[vertexIndex[9] * 3 + 2])
	: vec3(0);
#endif

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

layout(location = INSTANCE_PREV_ROW_0) in vec4 prev_world_row0;
layout(location = INSTANCE_PREV_ROW_1) in vec4 prev_world_row1;
layout(location = INSTANCE_PREV_ROW_2) in vec4 prev_world_row2;

mat4 worldMatrix = transpose(mat4(world_row0, world_row1, world_row2, vec4(0, 0, 0, 1)));
mat4 prevWorldMatrix = transpose(mat4(prev_world_row0, prev_world_row1, prev_world_row2, vec4(0, 0, 0, 1)));

#else

layout(binding = BINDING_OBJECT)
uniform Object_DYN_UNIFORM
{
	mat4 model;
	mat4 prev_model;
} object;

mat4 worldMatrix = object.model;
mat4 prevWorldMatrix = object.prev_model;

#endif // INSTANCE_INPUT

#endif // GPU_SCENE

#endif // VERTEX_INPUT_H