#version 450
#extension GL_ARB_separate_shader_objects : enable
layout(early_fragment_tests) in;

layout(location = 0) in vec2 uv;
layout(location = 1) in vec4 inWorldPos;
layout(location = 2) in vec4 inViewPos;

layout(location = 0) out vec4 outColor;

#include "public.glh"

layout(binding = TEXTURE_SLOT0) uniform sampler2D texSampler;

layout(binding = TEXTURE_SLOT1) uniform sampler2D cascadedShadowSampler0;
layout(binding = TEXTURE_SLOT2) uniform sampler2D cascadedShadowSampler1;
layout(binding = TEXTURE_SLOT3) uniform sampler2D cascadedShadowSampler2;
layout(binding = TEXTURE_SLOT4) uniform sampler2D cascadedShadowSampler3;

#define ambient 0.4

const mat4 biasMat = mat4(
	0.5, 0.0, 0.0, 0.0,
	0.0, 0.5, 0.0, 0.0,
	0.0, 0.0, 1.0, 0.0,
	0.5, 0.5, 0.0, 1.0 );

float textureProj(uint cascaded, vec2 off)
{
	float shadow = 1.0;
	float dist = 0.0;

	vec4 shadowCoord = (biasMat * cascaded_shadow.light_view_proj[cascaded]) * inWorldPos;	

	if(cascaded == 0)
	{
		dist = texture( cascadedShadowSampler0, shadowCoord.xy + off ).r;
	}
	else if(cascaded == 1)
	{
		dist = texture( cascadedShadowSampler1, shadowCoord.xy + off ).r;
	}
	else if(cascaded == 2)
	{
		dist = texture( cascadedShadowSampler2, shadowCoord.xy + off ).r;
	}
	else if(cascaded == 3)
	{
		dist = texture( cascadedShadowSampler3, shadowCoord.xy + off ).r;
	}

	if ( shadowCoord.w > 0.0 && dist < shadowCoord.z ) 
	{
		shadow = ambient;
	}
	
	return shadow;
}

float filterPCF(uint cascaded)
{
	ivec2 texDim = ivec2(1,1);
	
	if(cascaded == 0)
	{
		texDim = textureSize(cascadedShadowSampler0, 0);
	}
	else if(cascaded == 1)
	{
		texDim = textureSize(cascadedShadowSampler1, 0);
	}
	else if(cascaded == 2)
	{
		texDim = textureSize(cascadedShadowSampler2, 0);
	}
	else if(cascaded == 3)
	{
		texDim = textureSize(cascadedShadowSampler3, 0);
	}
	
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
			shadowFactor += textureProj(cascaded, vec2(dx*x, dy*y));
			count++;
		}
	}
	return shadowFactor / count;
}

const bool pcf = true;
const bool debug_layer = false;

void main()
{
	outColor = texture(texSampler, uv);
	//if(outColor.a == 0.0)
	//{
	//	discard;
	//}

	float shadow = 0.0f;

	// Get cascade index for the current fragment's view position

	uint cascaded = 0;
	for(uint i = 0; i < cascaded_shadow.cascaded - 1; ++i)
	{
		if(inViewPos.z < cascaded_shadow.frustrum[i])
		{
			cascaded = i + 1;
		}
	}

	if(pcf)
	{
		shadow = filterPCF(cascaded);
	}
	else
	{
		shadow = textureProj(cascaded, vec2(0.0f, 0.0f));
	}

	outColor.rgb *= shadow;
	
	if(debug_layer)
	{
		switch(cascaded)
		{
				case 0 : 
					outColor.rgb *= vec3(1.0f, 0.25f, 0.25f);
					break;
				case 1 : 
					outColor.rgb *= vec3(0.25f, 1.0f, 0.25f);
					break;
				case 2 : 
					outColor.rgb *= vec3(0.25f, 0.25f, 1.0f);
					break;
				case 3 : 
					outColor.rgb *= vec3(1.0f, 1.0f, 0.25f);
					break;
		}
	}
}