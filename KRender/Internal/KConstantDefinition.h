#pragma once
#include "Interface/IKRenderConfig.h"
#include "Interface/IKBuffer.h"
#include "glm/glm.hpp"

#include <vector>

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
		glm::mat4 MODEL = glm::mat4(1.0f);
		glm::mat4 PRVE_MODEL = glm::mat4(1.0f);
	};

	struct CSM_OBJECT_INSTANCE
	{
		uint32_t CASCADED_INDEX = 0;
	};

	struct CSM_OBJECT
	{
		OBJECT MODEL;
		CSM_OBJECT_INSTANCE CASCADED_INDEX;
	};

	struct DEBUG
	{
		OBJECT MODEL;
		glm::vec4 COLOR = glm::vec4(0.0f);
	};

	struct CAMERA
	{
		glm::mat4 VIEW = glm::mat4(1.0f);
		glm::mat4 PROJ = glm::mat4(1.0f);
		glm::mat4 VIEW_INV = glm::mat4(1.0f);
		glm::mat4 PROJ_INV = glm::mat4(1.0f);
		glm::mat4 VIEW_PROJ = glm::mat4(1.0f);
		glm::mat4 PREV_VIEW_PROJ = glm::mat4(1.0f);
		glm::vec4 PARAMETERS = glm::vec4(0.0f);
		glm::vec4 FRUSTUM_PLANES[6] = {};
	};

	struct SHADOW
	{
		glm::mat4 LIGHT_VIEW = glm::mat4(1.0f);
		glm::mat4 LIGHT_PROJ = glm::mat4(1.0f);
		glm::vec4 PARAMETERS = glm::vec4(0.0f);
	};

	struct CASCADED_SHADOW
	{
		glm::mat4 LIGHT_VIEW[4] = {};
		glm::mat4 LIGHT_VIEW_PROJ[4] = {};
		glm::vec4 LIGHT_INFO[4] = {};
		glm::vec4 FRUSTUM_PLANES[24] = {};
		glm::vec4 SPLIT = glm::vec4(0.0f);
		glm::vec4 CENTER = glm::vec4(0.0f);
		uint32_t NUM_CASCADED = 0;
	};

	struct VOXEL
	{
		glm::mat4 VIEW_PROJ[3] = {};
		glm::mat4 VIEW_PROJ_INV[3] = {};
		glm::vec4 MINPOINT_SCALE = glm::vec4(0.0f);
		glm::vec4 MAXPOINT_SCALE = glm::vec4(0.0f);
		glm::uvec4 MISCS = glm::uvec4(0);
		glm::vec4 MISCS2 = glm::vec4(0.0f);
		glm::vec4 MISCS3 = glm::vec4(0.0f);
	};

	struct VOXEL_CLIPMAP
	{
		glm::mat4 VIEW_PROJ[18] = {};
		glm::mat4 VIEW_PROJ_INV[18] = {};
		glm::vec4 UPDATE_REGION_MIN[18] = {};
		glm::vec4 UPDATE_REGION_MAX[18] = {};
		glm::vec4 REIGION_MIN_AND_VOXELSIZE[9] = {};
		glm::vec4 REIGION_MAX_AND_EXTENT[9] = {};
		glm::uvec4 MISCS = glm::uvec4(0);
		glm::uvec4 MISCS2 = glm::uvec4(0);
		glm::vec4 MISCS3 = glm::vec4(0.0f);
		glm::vec4 MISCS4 = glm::vec4(0.0f);
	};

	struct GLOBAL
	{
		glm::vec4 SUN_LIGHT_DIRECTION_AND_PBR_MAX_REFLECTION_LOD = glm::vec4(0.0f);
		glm::uvec4 MISCS = glm::uvec4(0);
	};

	struct VIRTUAL_TEXTURE
	{
		glm::uvec4 DESCRIPTION = glm::uvec4(0);
		glm::uvec4 DESCRIPTION2 = glm::uvec4(0);
	};
}