#ifndef VERTEX_INPUT_H
#define VERTEX_INPUT_H

#ifndef INSTANCE_INPUT
#	define INSTANCE_INPUT 0
#endif

#include "public.h"

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

#endif //INSTANCE_INPUT

#endif //VERTEX_INPUT_H