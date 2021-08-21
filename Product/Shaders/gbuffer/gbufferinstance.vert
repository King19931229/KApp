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

layout(location = 0) out vec4 worldNormal;
layout(location = 1) out vec4 worldPos;
layout(location = 2) out vec4 prevWorldPos;

void main()
{
	mat4 model = transpose(mat4(world_row0, world_row1, world_row2, vec4(0, 0, 0, 1)));
	mat4 prev_model = transpose(mat4(world_prev_row0, world_prev_row1, world_prev_row2, vec4(0, 0, 0, 1)));

	worldPos = model * vec4(position, 1.0);
	prevWorldPos = prev_model * vec4(position, 1.0);
	worldNormal.rgb = normalize(mat3(model) * normal);
	worldNormal.a = -(camera.view * worldPos).a;

	gl_Position = camera.viewProj * model * vec4(position, 1.0);
}