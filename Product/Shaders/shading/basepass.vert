#include "vertexinput.h"

layout(location = 0) out vec2 out_texCoord;
layout(location = 1) out vec3 out_worldPos;
layout(location = 2) out vec3 out_prevWorldPos;
layout(location = 3) out vec3 out_worldNormal;
#if TANGENT_BINORMAL_INPUT
layout(location = 4) out vec3 out_tangent;
layout(location = 5) out vec3 out_binormal;
#endif

#if VERTEX_COLOR_INPUT0
layout(location = 6) out vec3 out_color0;
#endif
#if VERTEX_COLOR_INPUT1
layout(location = 7) out vec3 out_color1;
#endif
#if VERTEX_COLOR_INPUT2
layout(location = 8) out vec3 out_color2;
#endif
#if VERTEX_COLOR_INPUT3
layout(location = 9) out vec3 out_color3;
#endif
#if VERTEX_COLOR_INPUT4
layout(location = 10) out vec3 out_color4;
#endif
#if VERTEX_COLOR_INPUT5
layout(location = 11) out vec3 out_color5;
#endif

#ifdef GPU_SCENE
layout(location = 12) out flat uint out_darwIndex;
#endif

void main()
{
#if GPU_SCENE
	if (gl_InstanceIndex >=	MegaShaderState[gpuscene.megaShaderIndex].instanceCount || gl_VertexIndex >= indexCount)
	{
		gl_Position = vec4(1, 1, 1, -1);
		return;
	}
	out_darwIndex = darwIndex;
#endif

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

#if VERTEX_COLOR_INPUT0
	out_color0 = color0;
#endif

#if VERTEX_COLOR_INPUT1
	out_color1 = color1;
#endif

#if VERTEX_COLOR_INPUT2
	out_color2 = color2;
#endif

#if VERTEX_COLOR_INPUT3
	out_color3 = color3;
#endif

#if VERTEX_COLOR_INPUT4
	out_color4 = color4;
#endif

#if VERTEX_COLOR_INPUT5
	out_color5 = color5;
#endif

	gl_Position = camera.viewProj * worldPos;
}