layout(early_fragment_tests) in;

layout(location = 0) in vec2 uv;

layout(location = 1) in vec4 inWorldPos;

layout(location = 2) in vec4 inViewPos;
layout(location = 3) in vec4 inViewNormal;
layout(location = 4) in vec4 inViewLightDir;

layout(location = 0) out vec4 outColor;

#include "public.h"
#include "shadow.h"

layout(binding = BINDING_DIFFUSE) uniform sampler2D diffuseSampler;

void main()
{
	float NDotL = max(dot(inViewNormal, -inViewLightDir), 0.0);
	float ambient = 0.5;
	outColor = texture(diffuseSampler, uv) * (NDotL + ambient);	
	outColor *= calcCSM(inViewPos.xyz, inWorldPos.xyz);		
}