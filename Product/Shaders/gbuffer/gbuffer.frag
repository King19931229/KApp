layout(early_fragment_tests) in;

layout(location = 0) in vec4 worldNormal;
layout(location = 1) in vec4 worldPos;
layout(location = 2) in vec4 prevWorldPos;
layout(location = 3) in vec2 texcoord;
#if TANGENT_BINORMAL_INPUT
layout(location = 4) in vec4 worldTangent;
layout(location = 5) in vec4 worldBinormal;
#endif

layout(location = 0) out vec4 normal;
layout(location = 1) out vec4 position;
layout(location = 2) out vec2 motion;
layout(location = 3) out vec4 diffuse;
layout(location = 4) out vec4 specular;

#include "public.h"

layout(binding = BINDING_DIFFUSE) uniform sampler2D diffuseSampler;
layout(binding = BINDING_SPECULAR) uniform sampler2D specularSampler;
layout(binding = BINDING_NORMAL) uniform sampler2D normalSampler;

void main()
{
	// 0
#if (TANGENT_BINORMAL_INPUT && HAS_MATERIAL_TEXTURE2)
	vec4 normalmap = 2.0 * texture(normalSampler, texcoord) - vec4(1.0);
	normal.xyz =
	normalize(worldTangent.xyz) * normalmap.r +
	normalize(worldBinormal.xyz) * normalmap.g +
	normalize(worldNormal.xyz) * normalmap.b;
#else
	normal.xyz = worldNormal.xyz;
#endif
	normal.a = worldNormal.a;
	// 1
	position = worldPos;
	// 2
	vec4 prev = camera.prevViewProj * prevWorldPos;
	vec4 curr = camera.viewProj * worldPos;
	vec2 prevUV = 0.5 * (prev.xy / prev.w + vec2(1, 1));
	vec2 currUV = 0.5 * (curr.xy / curr.w + vec2(1, 1));
	motion = currUV - prevUV;
	// 3
#if HAS_MATERIAL_TEXTURE0
	diffuse = texture(diffuseSampler, texcoord);
#else
	diffuse = vec4(0);
#endif
	// 4
#if HAS_MATERIAL_TEXTURE1
	specular = texture(specularSampler, texcoord);
#else
	specular = vec4(0);
#endif
}