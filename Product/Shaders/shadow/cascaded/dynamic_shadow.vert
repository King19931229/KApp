#include "public.h"

layout(location = POSITION) in vec3 position;
layout(location = NORMAL) in vec3 normal;
layout(location = TEXCOORD0) in vec2 texcoord0;

layout(location = 0) out vec2 out_texCoord;

#if !INSTANCE_INPUT
layout(binding = BINDING_OBJECT)
uniform Object
{
	mat4 model;
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
uniform Object
{
	uint index;
} object;
#endif

void main()
{
#if !INSTANCE_INPUT
	gl_Position = dynamic_cascaded.light_view_proj[object.index] * object.model * vec4(position, 1.0);
#else
	gl_Position = dynamic_cascaded.light_view_proj[object.index] * transpose(mat4(world_row0, world_row1, world_row2, vec4(0, 0, 0, 1))) * vec4(position, 1.0);
#endif
	out_texCoord = texcoord0;
}