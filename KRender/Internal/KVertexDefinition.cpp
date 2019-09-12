#include "KVertexDefinition.h"

#define MEMBER_OFFSET(structure, member) ((int)&((structure*)0)->member)

namespace KVertexDefinition
{
	int SemanticOffsetInElement(VertexSemantic semantic, VertexElement element)
	{
		if(element == VE_POINT_NORMAL_UV)
		{
			if(semantic == VS_POSITION)
			{
				return MEMBER_OFFSET(KVertexPositionNormalUV, poistion);
			}
			else if(semantic == VS_NORMAL)
			{
				return MEMBER_OFFSET(KVertexPositionNormalUV, normal);
			}
			else if(semantic == VS_UV)
			{
				return MEMBER_OFFSET(KVertexPositionNormalUV, uv);
			}
		}
		else if(element == VE_UV2)
		{
			if(semantic == VS_UV2)
			{
				return MEMBER_OFFSET(KVertexUV2, uv2);
			}
		}
		else if(element == VE_DIFFUSE_SPECULAR)
		{
			if(semantic == VS_DIFFUSE)
			{
				return MEMBER_OFFSET(KVertexDiffuseSpecular, diffuse);
			}
			else if(semantic == VS_SPECULAR)
			{
				return MEMBER_OFFSET(KVertexDiffuseSpecular, specular);
			}
		}
		else if(element == VE_TANGENT_BINORMAL)
		{
			if(semantic == VS_TANGENT)
			{
				return MEMBER_OFFSET(KVertexTangentBinormal, tangent);
			}
			else if(semantic == VS_BINORMAL)
			{
				return MEMBER_OFFSET(KVertexTangentBinormal, binormal);
			}
		}
		else if(element == VE_BLEND_WEIGHTS_INDICES)
		{
			if(semantic == VS_BLEND_WEIGHTS)
			{
				return MEMBER_OFFSET(KVertexBlendWeightsBlendIndices, blendWeights);
			}
			else if(semantic == VS_BLEND_INDICES)
			{
				return MEMBER_OFFSET(KVertexBlendWeightsBlendIndices, blendIndices);
			}
		}
		assert(false && "vertex semantic is not found in vertex element");
		return -1;
	}

}