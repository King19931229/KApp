#pragma once
#include "Interface/IKRenderConfig.h"
#include "Interface/IKBuffer.h"
#include "glm/glm.hpp"

#include <vector>
#include <assert.h>

namespace KVertexDefinition
{
#pragma pack(push, 1)

	struct POS_3F_NORM_3F_UV_2F
	{
		glm::vec3 POSITION;
		glm::vec3 NORMAL;
		glm::vec2 UV;
	};

	struct UV2_2F
	{
		glm::vec2 UV2;
	};

	struct DIFF_2F_SPEC_2F
	{
		glm::vec3 DIFFUSE;
		glm::vec3 SPECULAR;
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
#pragma pack(pop)
	struct VertexSemanticDetail
	{
		VertexSemantic semantic;
		int offset;
	};
	typedef std::vector<VertexSemanticDetail> VertexSemanticDetailList;
	static const VertexSemanticDetailList& SemanticsDetail(VertexFormat format);

	struct VertexBindingDetail
	{
		IKVertexBufferPtr vertexBuffer;
		std::vector<VertexFormat> formats;

		VertexBindingDetail()
		{
			vertexBuffer = nullptr;
		}
	};
}