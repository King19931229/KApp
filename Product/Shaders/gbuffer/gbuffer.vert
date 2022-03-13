#include "public.h"
#include "vertexinput.h"

#if !INSTANCE_INPUT
layout(binding = BINDING_OBJECT)
uniform Object
{
	mat4 model;
	mat4 prev_model;
}object;
#endif

layout(location = 0) out vec4 worldNormal;
layout(location = 1) out vec4 worldPos;
layout(location = 2) out vec4 prevWorldPos;
layout(location = 3) out vec2 texcoord;
#if TANGENT_BINORMAL_INPUT
layout(location = 4) out vec4 worldTangent;
layout(location = 5) out vec4 worldBinormal;
#endif

void main()
{
	// world normal
	worldNormal.rgb = normalize(mat3(WORLD_MATRIX) * normal);
	// world position
	worldPos = WORLD_MATRIX * vec4(position, 1.0);
	// view depth
	vec4 viewPos = camera.view * worldPos;
	worldNormal.a = -viewPos.z / viewPos.w;
	// prev world position
	prevWorldPos = PREV_WORLD_MATRIX * vec4(position, 1.0);
	// uv
	texcoord = texcoord0;
#if TANGENT_BINORMAL_INPUT
	// world tangent
	worldTangent = WORLD_MATRIX * vec4(tangent, 0.0);
	// world binormal
	worldBinormal = WORLD_MATRIX * vec4(binormal, 0.0);
#endif
	gl_Position = camera.viewProj * WORLD_MATRIX * vec4(position, 1.0);
}