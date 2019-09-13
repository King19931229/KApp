#include "KVertexDefinition.h"

#define MEMBER_OFFSET(structure, member) ((int)&((structure*)0)->member)

namespace KVertexDefinition
{
	static VertexSemanticDetailList POS_3F_NORM_3F_UV_2F_DETAILS;
	static VertexSemanticDetailList UV2_2F_DETAILS;
	static VertexSemanticDetailList DIFF_2F_SPEC_2F_DETAILS;
	static VertexSemanticDetailList TAN_3F_BIN_3F_DETAILS;
	static VertexSemanticDetailList BW_4F_BI_4I_DETAILS;
	static VertexSemanticDetailList EMPYT_DETAILS;
	static bool SEMANTIC_LIST_INIT = false;

	void SafeInit()
	{
		if(!SEMANTIC_LIST_INIT)
		{
			// POS_3F_NORM_3F_UV_2F
			{
				{
					VertexSemanticDetail DETAIL = {VS_POSITION, MEMBER_OFFSET(POS_3F_NORM_3F_UV_2F, POSITION)};
					POS_3F_NORM_3F_UV_2F_DETAILS.push_back(DETAIL);
				}
				{
					VertexSemanticDetail DETAIL = {VS_NORMAL, MEMBER_OFFSET(POS_3F_NORM_3F_UV_2F, NORMAL)};
					POS_3F_NORM_3F_UV_2F_DETAILS.push_back(DETAIL);
				}
				{
					VertexSemanticDetail DETAIL = {VS_UV, MEMBER_OFFSET(POS_3F_NORM_3F_UV_2F, UV)};
					POS_3F_NORM_3F_UV_2F_DETAILS.push_back(DETAIL);
				}
			}
			// UV2_2F
			{
				{
					VertexSemanticDetail DETAIL = {VS_UV2, MEMBER_OFFSET(UV2_2F, UV2)};
					UV2_2F_DETAILS.push_back(DETAIL);
				}
			}
			// DIFF_2F_SPEC_2F
			{
				{
					VertexSemanticDetail DETAIL = {VS_DIFFUSE, MEMBER_OFFSET(DIFF_2F_SPEC_2F, DIFFUSE)};
					DIFF_2F_SPEC_2F_DETAILS.push_back(DETAIL);
				}
				{
					VertexSemanticDetail DETAIL = {VS_SPECULAR, MEMBER_OFFSET(DIFF_2F_SPEC_2F, SPECULAR)};
					DIFF_2F_SPEC_2F_DETAILS.push_back(DETAIL);
				}
			}
			// TAN_3F_BIN_3F
			{
				{
					VertexSemanticDetail DETAIL = {VS_TANGENT, MEMBER_OFFSET(TAN_3F_BIN_3F, TANGENT)};
					TAN_3F_BIN_3F_DETAILS.push_back(DETAIL);
				}
				{
					VertexSemanticDetail DETAIL = {VS_BINORMAL, MEMBER_OFFSET(TAN_3F_BIN_3F, BINORMAL)};
					TAN_3F_BIN_3F_DETAILS.push_back(DETAIL);
				}
			}
			// BW_4F_BI_4I
			{
				{
					VertexSemanticDetail DETAIL = {VS_BLEND_WEIGHTS, MEMBER_OFFSET(BW_4F_BI_4I, BLEND_WEIGHTS)};
					BW_4F_BI_4I_DETAILS.push_back(DETAIL);
				}
				{
					VertexSemanticDetail DETAIL = {VS_BLEND_INDICES, MEMBER_OFFSET(BW_4F_BI_4I, BLEND_INDICES)};
					BW_4F_BI_4I_DETAILS.push_back(DETAIL);
				}
			}
			SEMANTIC_LIST_INIT = true;
		}
	}

	const VertexSemanticDetailList& SemanticsDetail(VertexFormat format)
	{
		SafeInit();
		switch (format)
		{
		case VF_POINT_NORMAL_UV:
			return POS_3F_NORM_3F_UV_2F_DETAILS;
		case VF_UV2:
			return UV2_2F_DETAILS;
		case VF_DIFFUSE_SPECULAR:
			return DIFF_2F_SPEC_2F_DETAILS;
		case VF_TANGENT_BINORMAL:
			return TAN_3F_BIN_3F_DETAILS;
		case VF_BLEND_WEIGHTS_INDICES:
			return BW_4F_BI_4I_DETAILS;
		default:
			return EMPYT_DETAILS;
		}
	}
}