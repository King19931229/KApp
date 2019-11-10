#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 uv;
layout(location = 1) in vec3 worldPos;
layout(location = 2) in vec3 worldNormal;
layout(location = 3) in vec3 worldEye;

layout(binding = 1) uniform sampler2D texSampler;
layout(binding = 2) uniform samplerCube samplerEnvMap;

layout(location = 0) out vec4 outColor;

#include "public.glh"

void main()
{
	vec3 eye = normalize(worldEye - worldPos);
	float n_dot_e = dot(worldNormal, eye);

	vec3 reflect = eye - 2.0 * worldNormal * n_dot_e;

	vec4 baseColor = texture(texSampler, uv);
	vec4 reflectColor = texture(samplerEnvMap, reflect);
	
	n_dot_e = abs(n_dot_e);

	outColor = vec4(baseColor.rgb * n_dot_e + reflectColor.rgb * (1.0 - n_dot_e), 1.0);
}