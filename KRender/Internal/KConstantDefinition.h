#pragma once
#include "Interface/IKRenderConfig.h"
#include "Interface/IKBuffer.h"
#include "glm/glm.hpp"

#include <vector>
#include <assert.h>

namespace KConstantDefinition
{
	struct TRANSFORM
	{
		glm::mat4 MODEL;
		glm::mat4 VIEW;
		glm::mat4 PROJ;
	};

	struct ConstantSemanticDetail
	{
		ConstantSemantic semantic;
		ElementFormat elementFormat;
		int elementCount;
		int offset;
	};
	typedef std::vector<ConstantSemanticDetail> ConstantSemanticDetailList;

	struct ConstantBufferDetail
	{
		ConstantSemanticDetailList semanticDetails;
		size_t bufferSize;
	};
	const ConstantBufferDetail& GetConstantBufferDetail(ConstantBufferType bufferType);

	struct ConstantBindingDetail
	{
		IKUniformBufferPtr constantBuffer;
		ConstantBufferDetail constantDetail;

		ConstantBindingDetail()
		{
			constantBuffer = nullptr;
		}
	};
}