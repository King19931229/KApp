#pragma once
#include "Interface/IKRenderConfig.h"
#include "Interface/IKBuffer.h"
#include "glm/glm.hpp"

#include <vector>

namespace KVertexDefinition
{
	struct POS_3F_NORM_3F_UV_2F
	{
		glm::vec3 POSITION;
		glm::vec3 NORMAL;
		glm::vec2 UV;
	};

	struct VIRTUAL_GEOMERTY_VERTEX_POS_3F_NORM_3F_UV_2F
	{
		glm::vec3 POSITION;
		glm::vec3 NORMAL;
		glm::vec2 UV;
	};

	struct UV2_2F
	{
		glm::vec2 UV2;
	};

	struct COLOR_3F
	{
		glm::vec3 COLOR;
	};

	struct TAN_3F_BIN_3F
	{
		glm::vec3 TANGENT;
		glm::vec3 BINORMAL;
	};

	struct BW_4F_BI_4I
	{
		glm::vec4 BLEND_WEIGHTS;
		glm::ivec4 BLEND_INDICES;
	};

	struct GUI_POS_2F_UV_2F_COLOR_4BYTE
	{
		glm::vec2 GUI_POSITION;
		glm::vec2 GUI_UV;
		uint32_t GUI_COLOR;
	};

	struct SCREENQUAD_POS_2F
	{
		glm::vec2 QUAD_POSITION;
	};

	struct DEBUG_POS_3F
	{
		glm::vec3 DEBUG_POSITION;
	};

	struct INSTANCE_DATA_MATRIX4F
	{
		glm::vec4 ROW0;
		glm::vec4 ROW1;
		glm::vec4 ROW2;
		glm::vec4 PREV_ROW0;
		glm::vec4 PREV_ROW1;
		glm::vec4 PREV_ROW2;

		INSTANCE_DATA_MATRIX4F()
			: ROW0(glm::vec4(1, 0, 0, 0)), ROW1(glm::vec4(0, 1, 0, 0)), ROW2(glm::vec4(0, 0, 1, 0))
			, PREV_ROW0(glm::vec4(1, 0, 0, 0)), PREV_ROW1(glm::vec4(0, 1, 0, 0)), PREV_ROW2(glm::vec4(0, 0, 1, 0))
		{}

		INSTANCE_DATA_MATRIX4F
		(
			const glm::vec4& row0, const glm::vec4& row1, const glm::vec4& row2,
			const glm::vec4& prev_row0, const glm::vec4& prev_row1, const glm::vec4& prev_row2
		)
			: ROW0(row0), ROW1(row1), ROW2(row2)
			, PREV_ROW0(prev_row0), PREV_ROW1(prev_row1), PREV_ROW2(prev_row2)
		{}
	};

	struct TERRAIN_POS_2F
	{
		glm::vec2 POS;
	};

	struct VertexSemanticDetail
	{
		VertexSemantic semantic;
		ElementFormat elementFormat;
		size_t offset;
	};
	typedef std::vector<VertexSemanticDetail> VertexSemanticDetailList;

	struct VertexDetail
	{
		VertexSemanticDetailList semanticDetails;
		size_t vertexSize;
	};
	const VertexDetail& GetVertexDetail(VertexFormat format);	
}