#pragma once
#include "Interface/IKRenderConfig.h"
#include <assert.h>

#define MEMBER_OFFSET(structure, member) ((int)&((structure*)0)->member)

#pragma pack(push, 1)
struct KVertexPositionNormalUV
{
	// vec3 poistion
	// vec3 normal
	// vec2 uv
};
struct KVertexUV2
{
	// vec2 uv
};
struct KVertexDiffuseSpecular
{
	// vec3 diffuse
	// vec3 specular
};
struct KVertexBlendWeightsBlendIndices
{
	// vec4 blendWeights
	// int4 blendIndices
};
#pragma pack(pop)

namespace KVertexDefinition
{
	static int SemanticOffset(VertexElement element, VertexSemantic semantic)
	{
		if(element == VE_POINT_NORMAL_UV)
		{
			if(semantic == VS_POSITION)
			{
			}
			else if(semantic == VS_NORMAL)
			{
			}
			else if(semantic == VS_UV)
			{
			}
		}
		else if(element == VE_UV2)
		{
			if(semantic == VS_UV2)
			{
			}
		}
		else if(element == VE_DIFFUSE_SPECULAR)
		{
			if(semantic == VS_DIFFUSE)
			{
			}
			else if(semantic == VS_SPECULAR)
			{
			}
		}
		else if(element == VE_TANGENT_BINORMAL)
		{
			if(semantic == VS_TANGENT)
			{
			}
			else if(semantic == VS_BINORMAL)
			{
			}
		}
		else if(element == VE_BLEND_WEIGHTS_INDICES)
		{
			if(semantic == VS_BLEND_WEIGHTS)
			{
			}
			else if(semantic == VS_BLEND_INDICES)
			{
			}
		}
		assert(false && "vertex semantic is not found in vertex element");
		return -1;
	}
}