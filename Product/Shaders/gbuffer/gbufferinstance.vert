#include "public.h"

layout(location = POSITION) in vec3 position;
layout(location = NORMAL) in vec3 normal;
layout(location = TEXCOORD0) in vec2 texcoord0;

layout(location = INSTANCE_COLUMN_0) in vec4 world_col0;
layout(location = INSTANCE_COLUMN_1) in vec4 world_col1;
layout(location = INSTANCE_COLUMN_2) in vec4 world_col2;
layout(location = INSTANCE_COLUMN_3) in vec4 world_col3;

layout(location = 0) out vec3 worldNormal;

void main()
{
	mat4 model =  mat4(world_col0, world_col1, world_col2, world_col3);
	worldNormal = mat3(model) * normal;
	gl_Position = camera.proj * camera.view * model * vec4(position, 1.0);
}