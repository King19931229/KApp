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
layout(location = 2) out vec2 vel;

void main()
{
	worldPos = object.model * vec4(position, 1.0);

	vec4 curPos =  camera.proj * camera.view * worldPos;
	vec4 prevPos = camera.prevViewProj * object.prev_model * vec4(position, 1.0);

	worldNormal.rgb = mat3(object.model) * normal;
	worldNormal.a = curPos.z / curPos.w;
	vel = (prevPos.xy / prevPos.w - curPos.xy / curPos.w) * 0.5;
	gl_Position = curPos;
}