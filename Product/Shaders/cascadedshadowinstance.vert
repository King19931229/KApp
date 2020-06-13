#version 450
#extension GL_ARB_separate_shader_objects : enable
#include "public.h"

layout(location = POSITION) in vec3 position;
layout(location = NORMAL) in vec3 normal;
layout(location = TEXCOORD0) in vec2 texcoord0;

layout(location = INSTANCE_COLUMN_0) in vec4 world_col0;
layout(location = INSTANCE_COLUMN_1) in vec4 world_col1;
layout(location = INSTANCE_COLUMN_2) in vec4 world_col2;
layout(location = INSTANCE_COLUMN_3) in vec4 world_col3;

layout(binding = BINDING_OBJECT)
uniform Object
{
	uint index;
}object;

void main()
{
	gl_Position = cascaded_shadow.light_view_proj[object.index] * mat4(world_col0, world_col1, world_col2, world_col3) * vec4(position, 1.0);
}