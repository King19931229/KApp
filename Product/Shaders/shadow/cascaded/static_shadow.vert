#include "public.h"

#ifdef GPU_SCENE
#include "shading/vertexinput.h"
layout(location = GPUSCENE_DRAW_INDEX_LOCATION) out flat uint out_darwIndex;
#else
layout(location = POSITION) in vec3 position;
layout(location = NORMAL) in vec3 normal;
layout(location = TEXCOORD0) in vec2 texcoord0;
#endif

layout(location = 0) out vec2 out_texCoord;

#if !defined(GPU_SCENE)
#if !INSTANCE_INPUT
layout(binding = BINDING_OBJECT)
uniform Object_DYN_UNIFORM
{
	mat4 model;
	mat4 prev_model;
	uint index;
} object;
#else
layout(location = INSTANCE_ROW_0) in vec4 world_row0;
layout(location = INSTANCE_ROW_1) in vec4 world_row1;
layout(location = INSTANCE_ROW_2) in vec4 world_row2;
layout(location = INSTANCE_PREV_ROW_0) in vec4 prev_world_row0;
layout(location = INSTANCE_PREV_ROW_1) in vec4 prev_world_row1;
layout(location = INSTANCE_PREV_ROW_2) in vec4 prev_world_row2;
layout(binding = BINDING_OBJECT)
uniform Object_DYN_UNIFORM
{
	uint index;
} object;
#endif
#endif

void main()
{
#ifdef GPU_SCENE
	out_darwIndex = darwIndex;
	gl_Position = static_cascaded.light_view_proj[gpuscene.cascadeIndex] * worldMatrix * vec4(position, 1.0);
#elif !INSTANCE_INPUT
	gl_Position = static_cascaded.light_view_proj[object.index] * object.model * vec4(position, 1.0);
#else
	gl_Position = static_cascaded.light_view_proj[object.index] * transpose(mat4(world_row0, world_row1, world_row2, vec4(0, 0, 0, 1))) * vec4(position, 1.0);
#endif
	out_texCoord = texcoord0;
}