#include "public.h"

layout(location = POSITION) in vec3 position;
layout(location = NORMAL) in vec3 normal;
layout(location = TEXCOORD0) in vec2 texcoord0;

layout(binding = BINDING_OBJECT)
uniform Object
{
	mat4 model;
	mat4 prev_model;
}object;

layout(location = 0) out vec4 worldNormal;
layout(location = 1) out vec4 worldPos;
layout(location = 2) out vec4 prevWorldPos;

void main()
{
	worldPos = object.model * vec4(position, 1.0);
	prevWorldPos = object.prev_model * vec4(position, 1.0);
	worldNormal.rgb = normalize(mat3(object.model) * normal);
	worldNormal.a = -(camera.view * worldPos).a;

	gl_Position = camera.viewProj * object.model * vec4(position, 1.0);
}