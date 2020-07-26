layout(early_fragment_tests) in;

layout(location = 0) in vec2 uv;

layout(location = 1) in vec4 inWorldPos;

layout(location = 2) in vec4 inViewPos;
layout(location = 3) in vec4 inViewNormal;
layout(location = 4) in vec4 inViewLightDir;

#if TANGENT_BINORMAL_INPUT
layout(location = 5) in vec4 inViewTangent;
layout(location = 6) in vec4 inViewBinormal;
#endif

layout(location = 0) out vec4 outColor;

#include "public.h"
#include "shadow.h"

layout(binding = BINDING_DIFFUSE) uniform sampler2D diffuseSampler;
layout(binding = BINDING_NORMAL) uniform sampler2D normalSampler;

void main()
{
	vec4 viewNormal;
#if(TANGENT_BINORMAL_INPUT && HAS_MATERIAL_TEXTURE2)
	vec4 normalmap = 2.0 * texture(normalSampler, uv) - vec4(1.0);
	viewNormal =
	normalize(inViewTangent) * normalmap.r +
	normalize(inViewBinormal) * normalmap.g +
	normalize(inViewNormal) * normalmap.b;
#else
	viewNormal = inViewNormal;
#endif

	vec4 diffuse;
#if HAS_MATERIAL_TEXTURE0
	diffuse = texture(diffuseSampler, uv);
#else
	diffuse = vec4(1,0,0,1);
#endif

	float NDotL = max(dot(viewNormal, -inViewLightDir), 0.0);
	float ambient = 0.5;
	outColor = diffuse * (NDotL + ambient);	
	outColor *= calcCSM(inViewPos.xyz, inWorldPos.xyz);
}