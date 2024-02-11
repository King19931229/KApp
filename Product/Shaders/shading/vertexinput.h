#ifndef VERTEX_INPUT_H
#define VERTEX_INPUT_H

#include "public.h"

#ifdef GPU_SCENE

#include "gpuscene/gpuscene_define.h"

layout(std430, binding = BINDING_POINT_NORMAL_UV) readonly buffer PointNormalUVPackBuffer { float PointNormalUVData[]; };

#if TANGENT_BINORMAL_INPUT
layout(std430, binding = BINDING_TANGENT_BINORMAL) readonly buffer TangentBinormalPackBuffer { float TangentBinormalData[]; };
#endif

#if BLEND_WEIGHT_INPUT
layout(std430, binding = BINDING_BLEND_WEIGHTS_INDICES) readonly buffer BlendWeightsIndicesPackBuffer { float BlendWeightsIndicesData[]; };
#endif

#if UV2_INPUT
layout(std430, binding = BINDING_UV2) readonly buffer UV2PackBuffer { float UV2Data[]; };
#endif

#if VERTEX_COLOR_INPUT0
layout(std430, binding = BINDING_COLOR0) readonly buffer Color0PackBuffer { float Color0Data[]; };
#endif
#if VERTEX_COLOR_INPUT1
layout(std430, binding = BINDING_COLOR1) readonly buffer Color1PackBuffer { float Color1Data[]; };
#endif
#if VERTEX_COLOR_INPUT2
layout(std430, binding = BINDING_COLOR2) readonly buffer Color2PackBuffer { float Color2Data[]; };
#endif
#if VERTEX_COLOR_INPUT3
layout(std430, binding = BINDING_COLOR3) readonly buffer Color3PackBuffer { float Color3Data[]; };
#endif
#if VERTEX_COLOR_INPUT4
layout(std430, binding = BINDING_COLOR4) readonly buffer Color4PackBuffer { float Color4Data[]; };
#endif
#if VERTEX_COLOR_INPUT5
layout(std430, binding = BINDING_COLOR5) readonly buffer Color5PackBuffer { float Color5Data[]; };
#endif

layout(std430, binding = BINDING_INDEX) readonly buffer IndexPackBuffer { uint IndexData[]; };

layout(std430, binding = BINDING_INSTANCE_DATA) readonly buffer InstanceDataPackBuffer { InstanceStruct InstanceData[]; };

// layout(std430, binding = BINDING_MATERIAL_PARAMETER) readonly buffer MaterialParameterPackBuffer { float MaterialParameterData[]; };
layout(std430, binding = BINDING_DRAWING_INSTANCE) readonly buffer DrawingInstancePackBuffer { DrawingInstanceStruct DrawingInstanceData[]; };

uint instanceIndex = DrawingInstanceData[gl_InstanceIndex].data[0];
uint meshIndex = DrawingInstanceData[gl_InstanceIndex].data[1];
uint shaderIndex = DrawingInstanceData[gl_InstanceIndex].data[2];
uint darwIndex = DrawingInstanceData[gl_InstanceIndex].data[3];

mat4 worldMatrix = InstanceData[instanceIndex].transform;
mat4 prevWorldMatrix = InstanceData[instanceIndex].prevTransform;

vec3 position = vec3(PointNormalUVData[meshIndex * 8],PointNormalUVData[meshIndex * 8 + 1],PointNormalUVData[meshIndex * 8 + 2]);
vec3 normal = vec3(PointNormalUVData[meshIndex * 8 + 3],PointNormalUVData[meshIndex * 8 + 4],PointNormalUVData[meshIndex * 8 + 5]);
vec2 texcoord0 = vec2(PointNormalUVData[meshIndex * 8 + 6],PointNormalUVData[meshIndex * 8 + 7]);

#if TANGENT_BINORMAL_INPUT
vec3 tangent = vec3(TangentBinormalData[meshIndex * 6],TangentBinormalData[meshIndex * 6 + 1],TangentBinormalData[meshIndex * 6 + 2]);
vec3 binormal = vec3(TangentBinormalData[meshIndex * 6 + 3],TangentBinormalData[meshIndex * 6 + 4],TangentBinormalData[meshIndex * 6 + 5]);
#endif

#if UV2_INPUT
vec2 texcoord1 = vec2(UV2Data[meshIndex * 2], UV2Data[meshIndex * 2 + 1]);
#endif

#if VERTEX_COLOR_INPUT0
vec3 color0 = vec3(Color0Data[meshIndex * 3],Color0Data[meshIndex * 3 + 1],Color0Data[meshIndex * 3 + 2]);
#endif
#if VERTEX_COLOR_INPUT1
vec3 color1 = vec3(Color1Data[meshIndex * 3],Color1Data[meshIndex * 3 + 1],Color1Data[meshIndex * 3 + 2]);
#endif
#if VERTEX_COLOR_INPUT2
vec3 color2 = vec3(Color2Data[meshIndex * 3],Color2Data[meshIndex * 3 + 1],Color2Data[meshIndex * 3 + 2]);
#endif
#if VERTEX_COLOR_INPUT3
vec3 color3 = vec3(Color3Data[meshIndex * 3],Color3Data[meshIndex * 3 + 1],Color3Data[meshIndex * 3 + 2]);
#endif
#if VERTEX_COLOR_INPUT4
vec3 color4 = vec3(Color4Data[meshIndex * 3],Color4Data[meshIndex * 3 + 1],Color4Data[meshIndex * 3 + 2]);
#endif
#if VERTEX_COLOR_INPUT5
vec3 color5 = vec3(Color5Data[meshIndex * 3],Color5Data[meshIndex * 3 + 1],Color5Data[meshIndex * 3 + 2]);
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