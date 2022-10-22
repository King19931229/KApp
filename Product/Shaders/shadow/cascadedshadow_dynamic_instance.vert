#include "public.h"
#define cascaded_shadow dynamic_cascaded

layout(location = POSITION) in vec3 position;
layout(location = NORMAL) in vec3 normal;
layout(location = TEXCOORD0) in vec2 texcoord0;

layout(location = INSTANCE_ROW_0) in vec4 world_row0;
layout(location = INSTANCE_ROW_1) in vec4 world_row1;
layout(location = INSTANCE_ROW_2) in vec4 world_row2;

layout(binding = BINDING_OBJECT)
uniform Object
{
	uint index;
}object;

void main()
{
	gl_Position = cascaded_shadow.light_view_proj[object.index] * transpose(mat4(world_row0, world_row1, world_row2, vec4(0, 0, 0, 1))) * vec4(position, 1.0);
}