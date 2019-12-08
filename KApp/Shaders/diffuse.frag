#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 uv;
layout(location = 1) in vec4 shadowCoord;
layout(location = 0) out vec4 outColor;

#include "public.glh"

layout(binding = TEXTURE_SLOT0) uniform sampler2D texSampler;
layout(binding = TEXTURE_SLOT3) uniform sampler2D shadowSampler;

#define ambient 0.4
float textureProj(vec4 shadowCoord, vec2 off)
{
	float shadow = 1.0;
	if ( shadowCoord.z > -1.0 && shadowCoord.z < 1.0 ) 
	{
		float dist = texture( shadowSampler, shadowCoord.xy + off ).r;
		if ( shadowCoord.w > 0.0 && dist < shadowCoord.z ) 
		{
			shadow = ambient;
		}
	}
	else
	{
		shadow = 0.1;
	}
	return shadow;
}

float filterPCF(vec4 sc)
{
	ivec2 texDim = textureSize(shadowSampler, 0);
	float scale = 1.0;
	float dx = scale * 1.0 / float(texDim.x);
	float dy = scale * 1.0 / float(texDim.y);

	float shadowFactor = 0.0;
	int count = 0;
	int range = 1;
	
	for (int x = -range; x <= range; x++)
	{
		for (int y = -range; y <= range; y++)
		{
			shadowFactor += textureProj(sc, vec2(dx*x, dy*y));
			count++;
		}
	
	}
	return shadowFactor / count;
}

float LinearizeDepth(float depth)
{
	float n = shadow.near_far.x; // camera z near
	float f = shadow.near_far.y; // camera z far
	float z = depth;
	return (2.0 * n) / (f + n - z * (f - n));	
}

void main()
{
	float shadow = filterPCF(shadowCoord);
	//
	outColor = texture(texSampler, uv);
	outColor.rgb *= shadow;

	//float depth = texture(shadowSampler, shadowCoord.xy).r;
	//outColor = vec4(vec3(1.0 - LinearizeDepth(depth)), 1.0);	
}