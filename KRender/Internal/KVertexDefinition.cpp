#include "KVertexDefinition.h"
#include <algorithm>

namespace KVertexDefinition
{
	static VertexDetail POS_3F_NORM_3F_UV_2F_DETAILS;
	static VertexDetail UV2_2F_DETAILS;
	static VertexDetail DIFF_3F_SPEC_3F_DETAILS;
	static VertexDetail TAN_3F_BIN_3F_DETAILS;
	static VertexDetail BW_4F_BI_4I_DETAILS;
	static VertexDetail GUI_POS_2F_UV_2F_COLOR_4BYTE_DETAILS;
	static VertexDetail SCREENQUAD_POS_2F_DETAILS;
	static VertexDetail DEBUG_POS_3F_DETAILS;
	static VertexDetail EMPYT_DETAILS;
	static bool VERTEX_DETAIL_INIT = false;

	void SafeInit()
	{
		if(!VERTEX_DETAIL_INIT)
		{
			// POS_3F_NORM_3F_UV_2F
			{
				// VS_POSITION
				{
					VertexSemanticDetail DETAIL = {VS_POSITION, EF_R32G32B32_FLOAT, MEMBER_OFFSET(POS_3F_NORM_3F_UV_2F, POSITION)};
					POS_3F_NORM_3F_UV_2F_DETAILS.semanticDetails.push_back(DETAIL);
				}
				// VS_NORMAL
				{
					VertexSemanticDetail DETAIL = {VS_NORMAL, EF_R32G32B32_FLOAT, MEMBER_OFFSET(POS_3F_NORM_3F_UV_2F, NORMAL)};
					POS_3F_NORM_3F_UV_2F_DETAILS.semanticDetails.push_back(DETAIL);
				}
				// VS_UV
				{
					VertexSemanticDetail DETAIL = {VS_UV, EF_R32G32_FLOAT, MEMBER_OFFSET(POS_3F_NORM_3F_UV_2F, UV)};
					POS_3F_NORM_3F_UV_2F_DETAILS.semanticDetails.push_back(DETAIL);
				}
				POS_3F_NORM_3F_UV_2F_DETAILS.vertexSize = sizeof(POS_3F_NORM_3F_UV_2F);
			}
			// UV2_2F
			{
				// VS_UV2
				{
					VertexSemanticDetail DETAIL = {VS_UV2, EF_R32G32_FLOAT, MEMBER_OFFSET(UV2_2F, UV2)};
					UV2_2F_DETAILS.semanticDetails.push_back(DETAIL);
				}
				UV2_2F_DETAILS.vertexSize = sizeof(UV2_2F);
			}
			// DIFF_2F_SPEC_2F
			{
				// VS_DIFFUSE
				{
					VertexSemanticDetail DETAIL = {VS_DIFFUSE, EF_R32G32B32_FLOAT, MEMBER_OFFSET(DIFF_3F_SPEC_3F, DIFFUSE)};
					DIFF_3F_SPEC_3F_DETAILS.semanticDetails.push_back(DETAIL);
				}
				// VS_SPECULAR
				{
					VertexSemanticDetail DETAIL = {VS_SPECULAR, EF_R32G32B32_FLOAT, MEMBER_OFFSET(DIFF_3F_SPEC_3F, SPECULAR)};
					DIFF_3F_SPEC_3F_DETAILS.semanticDetails.push_back(DETAIL);
				}
				DIFF_3F_SPEC_3F_DETAILS.vertexSize = sizeof(DIFF_3F_SPEC_3F);
			}
			// TAN_3F_BIN_3F
			{
				// VS_TANGENT
				{
					VertexSemanticDetail DETAIL = {VS_TANGENT, EF_R32G32B32_FLOAT, MEMBER_OFFSET(TAN_3F_BIN_3F, TANGENT)};
					TAN_3F_BIN_3F_DETAILS.semanticDetails.push_back(DETAIL);
				}
				// VS_BINORMAL
				{
					VertexSemanticDetail DETAIL = {VS_BINORMAL, EF_R32G32B32_FLOAT, MEMBER_OFFSET(TAN_3F_BIN_3F, BINORMAL)};
					TAN_3F_BIN_3F_DETAILS.semanticDetails.push_back(DETAIL);
				}
				TAN_3F_BIN_3F_DETAILS.vertexSize = sizeof(TAN_3F_BIN_3F);
			}
			// BW_4F_BI_4I
			{
				// VS_BLEND_WEIGHTS
				{
					VertexSemanticDetail DETAIL = {VS_BLEND_WEIGHTS, EF_R32_FLOAT, MEMBER_OFFSET(BW_4F_BI_4I, BLEND_WEIGHTS)};
					BW_4F_BI_4I_DETAILS.semanticDetails.push_back(DETAIL);
				}
				// VS_BLEND_INDICES
				{
					VertexSemanticDetail DETAIL = {VS_BLEND_INDICES, EF_R32_UINT, MEMBER_OFFSET(BW_4F_BI_4I, BLEND_INDICES)};
					BW_4F_BI_4I_DETAILS.semanticDetails.push_back(DETAIL);
				}
				BW_4F_BI_4I_DETAILS.vertexSize = sizeof(BW_4F_BI_4I);
			}
			// GUI_POS_2F_UV_2F_COLOR_4BYTE_DETAILS
			{
				// VS_GUI_POS
				{
					VertexSemanticDetail DETAIL = {VS_GUI_POS, EF_R32G32_FLOAT, MEMBER_OFFSET(GUI_POS_2F_UV_2F_COLOR_4BYTE, GUI_POSITION)};
					GUI_POS_2F_UV_2F_COLOR_4BYTE_DETAILS.semanticDetails.push_back(DETAIL);
				}
				// VS_GUI_UV
				{
					VertexSemanticDetail DETAIL = {VS_GUI_UV, EF_R32G32_FLOAT, MEMBER_OFFSET(GUI_POS_2F_UV_2F_COLOR_4BYTE, GUI_UV)};
					GUI_POS_2F_UV_2F_COLOR_4BYTE_DETAILS.semanticDetails.push_back(DETAIL);
				}
				// VS_GUI_COLOR
				{
					VertexSemanticDetail DETAIL = {VS_GUI_COLOR, EF_R8GB8BA8_UNORM, MEMBER_OFFSET(GUI_POS_2F_UV_2F_COLOR_4BYTE, GUI_COLOR)};
					GUI_POS_2F_UV_2F_COLOR_4BYTE_DETAILS.semanticDetails.push_back(DETAIL);
				}
				GUI_POS_2F_UV_2F_COLOR_4BYTE_DETAILS.vertexSize = sizeof(GUI_POS_2F_UV_2F_COLOR_4BYTE);
			}
			// SCREENQUAD_POS_2F_DETAILS
			{
				// VS_SCREENQAUD_POS
				{
					VertexSemanticDetail DETAIL = {VS_SCREENQAUD_POS, EF_R32G32_FLOAT, MEMBER_OFFSET(SCREENQUAD_POS_2F, QUAD_POSITION)};
					SCREENQUAD_POS_2F_DETAILS.semanticDetails.push_back(DETAIL);
				}
				SCREENQUAD_POS_2F_DETAILS.vertexSize = sizeof(SCREENQUAD_POS_2F);
			}
			// DEBUG_POS_3F_DETAILS
			{
				// VS_POSITION
				{
					VertexSemanticDetail DETAIL = { VS_POSITION, EF_R32G32B32_FLOAT, MEMBER_OFFSET(DEBUG_POS_3F, DEBUG_POSITION) };
					DEBUG_POS_3F_DETAILS.semanticDetails.push_back(DETAIL);
				}
				DEBUG_POS_3F_DETAILS.vertexSize = sizeof(DEBUG_POS_3F);
			}

			VERTEX_DETAIL_INIT = true;
		}
	}

	const VertexDetail& GetVertexDetail(VertexFormat format)
	{
		SafeInit();
		switch (format)
		{
		case VF_POINT_NORMAL_UV:
			return POS_3F_NORM_3F_UV_2F_DETAILS;
		case VF_UV2:
			return UV2_2F_DETAILS;
		case VF_DIFFUSE_SPECULAR:
			return DIFF_3F_SPEC_3F_DETAILS;
		case VF_TANGENT_BINORMAL:
			return TAN_3F_BIN_3F_DETAILS;
		case VF_BLEND_WEIGHTS_INDICES:
			return BW_4F_BI_4I_DETAILS;
		case VF_GUI_POS_UV_COLOR:
			return GUI_POS_2F_UV_2F_COLOR_4BYTE_DETAILS;
		case VF_SCREENQUAD_POS:
			return SCREENQUAD_POS_2F_DETAILS;
		case VF_DEBUG_POINT:
			return DEBUG_POS_3F_DETAILS;
		default:
			assert(false && "unknown format");
			return EMPYT_DETAILS;
		}
	}
}