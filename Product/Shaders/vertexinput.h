#ifndef VERTEX_INPUT_H
#define VERTEX_INPUT_H

#include "public.h"

#ifdef GPU_SCENE

layout(std430, binding = BINDING_POINT_NORMAL_UV) readonly buffer PointNormalUVPackBuffer { float PointNormalUVPack[]; };
layout(std430, binding = BINDING_TANGENT_BINORMAL) readonly buffer TangentBinormalPackBuffer { float TangentBinormalPack[]; };

layout(std430, binding = BINDING_BLEND_WEIGHTS_INDICES) readonly buffer BlendWeightsIndicesPackBuffer { float BlendWeightsIndicesPack[]; };

layout(std430, binding = BINDING_UV2) readonly buffer UV2PackBuffer { float UV2Pack[]; };
layout(std430, binding = BINDING_COLOR0) readonly buffer Color0PackBuffer { float Color0PackBuffer[]; };

layout(std430, binding = BINDING_COLOR1) readonly buffer Color1PackBuffer { float Color1Pack[]; };
layout(std430, binding = BINDING_COLOR2) readonly buffer Color2PackBuffer { float Color2Pack[]; };
layout(std430, binding = BINDING_COLOR3) readonly buffer Color3PackBuffer { float Color3Pack[]; };
layout(std430, binding = BINDING_COLOR4) readonly buffer Color4PackBuffer { float Color4Pack[]; };
layout(std430, binding = BINDING_COLOR5) readonly buffer Color5PackBuffer { float Color5Pack[]; };

layout(std430, binding = BINDING_INDEX) readonly buffer IndexPackBuffer { uint IndexPack[]; };

layout(std430, binding = BINDING_INSTANCE_DATA) readonly buffer InstanceDataPackBuffer { float InstanceDataPack[]; };

layout(std430, binding = BINDING_MATERIAL_PARAMETER) readonly buffer MaterialParameterPackBuffer { uint MaterialParameterPack[]; };

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

#ifndef INSTANCE_INPUT
#	define INSTANCE_INPUT 0
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

#endif // GPU_SCENE

#endif // VERTEX_INPUT_H