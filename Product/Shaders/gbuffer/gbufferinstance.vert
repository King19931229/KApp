#include "public.h"

layout(location = POSITION) in vec3 position;
layout(location = NORMAL) in vec3 normal;
layout(location = TEXCOORD0) in vec2 texcoord0;

layout(location = INSTANCE_ROW_0) in vec4 world_row0;
layout(location = INSTANCE_ROW_1) in vec4 world_row1;
layout(location = INSTANCE_ROW_2) in vec4 world_row2;

layout(location = INSTANCE_PREV_ROW_0) in vec4 world_prev_row0;
layout(location = INSTANCE_PREV_ROW_1) in vec4 world_prev_row1;
layout(location = INSTANCE_PREV_ROW_2) in vec4 world_prev_row2;

layout(location = 0) out vec4 encoded0;
layout(location = 1) out vec4 encoded1;

void main()
{
	mat4 model = transpose(mat4(world_row0, world_row1, world_row2, vec4(0, 0, 0, 1)));
	encoded0.rgb = mat3(model) * normal;
	encoded1 = model * vec4(position, 1.0);
	gl_Position = camera.proj * camera.view * encoded1;
	encoded0.a = gl_Position.z / gl_Position.w;
}