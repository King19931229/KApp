#ifndef VERTEX_INPUT_H
#define VERTEX_INPUT_H

#ifndef INSTANCE_INPUT
#	define INSTANCE_INPUT 0
#endif

#include "public.h"

layout(location = POSITION) in vec3 position;
layout(location = NORMAL) in vec3 normal;
layout(location = TEXCOORD0) in vec2 texcoord0;

//layout(location = DIFFUSE) in vec3 diffuse;
//layout(location = SPECULAR) in vec3 specular;

//layout(location = TANGENT) in vec3 tangent;
//layout(location = BINORMAL) in vec3 binormal;

#if INSTANCE_INPUT

layout(location = INSTANCE_COLUMN_0) in vec4 world_col0;
layout(location = INSTANCE_COLUMN_1) in vec4 world_col1;
layout(location = INSTANCE_COLUMN_2) in vec4 world_col2;
layout(location = INSTANCE_COLUMN_3) in vec4 world_col3;

#define WORLD_MATRIX mat4(world_col0, world_col1, world_col2, world_col3)

#else

layout(binding = BINDING_OBJECT)
uniform Object
{
	mat4 model;
}object;

#define WORLD_MATRIX object.model

#endif //INSTANCE_INPUT

#endif //VERTEX_INPUT_H