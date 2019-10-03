#pragma once
#include "Interface/IKRenderConfig.h"
#include "Interface/IKBuffer.h"
#include "glm/glm.hpp"

#include <vector>
#include <assert.h>

/*
https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/chap14.html#interfaces-resources-layout
A scalar of size N has a scalar alignment of N.
A vector or matrix type has a scalar alignment equal to that of its component type.
An array type has a scalar alignment equal to that of its element type.
A structure has a scalar alignment equal to the largest scalar alignment of any of its members.
*/
namespace KConstantDefinition
{
	struct OBJECT
	{
		glm::mat4 MODEL;
	};

	struct CAMERA
	{
		glm::mat4 VIEW;
		glm::mat4 PROJ;
	};

	struct ConstantSemanticDetail
	{
		ConstantSemantic semantic;
		ElementFormat elementFormat;
		int elementCount;
		int size;
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
		int slot;

		ConstantBindingDetail()
		{
			constantBuffer = nullptr;
			slot = -1;
		}
	};
}