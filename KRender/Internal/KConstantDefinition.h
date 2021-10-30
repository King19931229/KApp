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
		glm::mat4 PRVE_MODEL;
	};

	struct CSM_OBJECT_INSTANCE
	{
		uint32_t CASCADED_INDEX;
	};

	struct CSM_OBJECT
	{
		OBJECT MODEL;
		CSM_OBJECT_INSTANCE CASCADED_INDEX;
	};

	struct DEBUG
	{
		OBJECT MODEL;
		glm::vec4 COLOR;
	};

	struct CAMERA
	{
		glm::mat4 VIEW;
		glm::mat4 PROJ;
		glm::mat4 VIEW_INV;
		glm::mat4 PROJ_INV;
		glm::mat4 VIEW_PROJ;
		glm::mat4 PREV_VIEW_PROJ;
		glm::vec4 PARAMETERS;
	};

	struct SHADOW
	{
		glm::mat4 LIGHT_VIEW;
		glm::mat4 LIGHT_PROJ;
		glm::vec4 PARAMETERS;
	};

	struct CASCADED_SHADOW
	{
		glm::mat4 LIGHT_VIEW[4];
		glm::mat4 LIGHT_VIEW_PROJ[4];
		glm::vec4 LIGHT_INFO[4];
		float FRUSTUM[4];
		uint32_t NUM_CASCADED;
	};

	struct VOXEL
	{
		glm::mat4 VIEW_PROJ[3];
		glm::mat4 VIEW_PROJ_INV[3];
		glm::vec4 MIDPOINT_SCALE;
		glm::uvec4 MISCS;
		glm::vec4 MISCS2;
	};

	struct GLOBAL
	{
		glm::vec4 SUN_LIGHT_DIR;
	};

	struct ConstantSemanticDetail
	{
		ConstantSemantic semantic;
		ElementFormat elementFormat;
		size_t elementCount;
		size_t size;
		size_t offset;
	};
	typedef std::vector<ConstantSemanticDetail> ConstantSemanticDetailList;

	struct ConstantBufferDetail
	{
		ConstantSemanticDetailList semanticDetails;
		size_t bufferSize;
	};
	const ConstantBufferDetail& GetConstantBufferDetail(ConstantBufferType bufferType);
}