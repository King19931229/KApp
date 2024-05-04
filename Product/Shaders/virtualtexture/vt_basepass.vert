#include "shading/vertexinput.h"

layout(location = 0) out vec2 out_texCoord;
layout(location = 1) out vec3 out_worldPos;
layout(location = 2) out vec3 out_prevWorldPos;
layout(location = 3) out vec3 out_worldNormal;
#if TANGENT_BINORMAL_INPUT
layout(location = 4) out vec3 out_tangent;
layout(location = 5) out vec3 out_binormal;
#endif

void main()
{
	vec4 worldNormal = normalize(worldMatrix * vec4(normal, 0.0));

#if TANGENT_BINORMAL_INPUT
	vec4 worldTangent = normalize(worldMatrix * vec4(tangent, 0.0));
	vec4 worldBinormal = normalize(worldMatrix * vec4(binormal, 0.0));
#endif

	vec4 worldPos = worldMatrix * vec4(position, 1.0);
	vec4 prevWorldPos = prevWorldMatrix * vec4(position, 1.0);

	out_texCoord = texcoord0;
	out_worldPos = worldPos.xyz;
	out_prevWorldPos = prevWorldPos.xyz;
	out_worldNormal = worldNormal.xyz;
#if TANGENT_BINORMAL_INPUT
	out_tangent = worldTangent.xyz;
	out_binormal = worldBinormal.xyz;
#endif

	gl_Position = camera.viewProj * worldPos;
}