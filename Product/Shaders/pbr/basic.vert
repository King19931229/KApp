#include "vertexinput.h"

layout(location = 0) out vec2 uv;
layout(location = 1) out vec4 worldPos;
layout(location = 2) out vec4 worldNormal;
layout(location = 3) out vec4 cameraPos;


#if !INSTANCE_INPUT
layout(binding = BINDING_OBJECT)
uniform Object
{
	mat4 model;
}object;
#endif

void main()
{
	uv = texcoord0;

	mat4 worldMatrix = WORLD_MATRIX;

#if TANGENT_BINORMAL_INPUT
	vec3 T = normalize(mat3(worldMatrix) * tangent);
	vec3 B = normalize(mat3(worldMatrix) * binormal);
	vec3 N = normalize(mat3(worldMatrix) * normal);
	mat3 TBN = transpose(mat3(T, B, N));
#else
	mat3 TBN = mat3(1.0);
#endif

	cameraPos = camera.viewInv * vec4(0.0, 0.0, 0.0, 1.0);
	cameraPos.xyzw /= cameraPos.w;
	worldPos = worldMatrix * vec4(position, 1.0);
	worldNormal = worldMatrix * vec4(normal, 1.0);

	gl_Position = camera.proj * camera.view * worldPos;
}