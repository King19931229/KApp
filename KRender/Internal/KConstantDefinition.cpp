#include "KConstantDefinition.h"

namespace KConstantDefinition
{
	static ConstantBufferDetail CAMERA_DETAILS;
	static ConstantBufferDetail SHADOW_DETAILS;
	static ConstantBufferDetail CASCADED_SHADOW_DETAILS;
	static ConstantBufferDetail GLOBAL_DETAILS;
	static ConstantBufferDetail VOXEL_DETAILS;
	static ConstantBufferDetail EMPYT_DETAILS;

	void SafeInit()
	{
		static bool CONSTANT_DETAIL_INIT = false;
		if(!CONSTANT_DETAIL_INIT)
		{
			// CAMERA
			{
				// VIEW
				{
					ConstantSemanticDetail DETAIL = { CS_VIEW, EF_R32G32B32A32_FLOAT, 4, MEMBER_SIZE(CAMERA, VIEW), MEMBER_OFFSET(CAMERA, VIEW) };
					CAMERA_DETAILS.semanticDetails.push_back(DETAIL);
				}
				// PROJ
				{
					ConstantSemanticDetail DETAIL = { CS_PROJ, EF_R32G32B32A32_FLOAT, 4, MEMBER_SIZE(CAMERA, PROJ), MEMBER_OFFSET(CAMERA, PROJ) };
					CAMERA_DETAILS.semanticDetails.push_back(DETAIL);
				}
				// VIEW_INV
				{
					ConstantSemanticDetail DETAIL = { CS_VIEW_INV, EF_R32G32B32A32_FLOAT, 4, MEMBER_SIZE(CAMERA, VIEW_INV), MEMBER_OFFSET(CAMERA, VIEW_INV) };
					CAMERA_DETAILS.semanticDetails.push_back(DETAIL);
				}
				// PROJ_INV
				{
					ConstantSemanticDetail DETAIL = { CS_PROJ_INV, EF_R32G32B32A32_FLOAT, 4, MEMBER_SIZE(CAMERA, PROJ_INV), MEMBER_OFFSET(CAMERA, PROJ_INV) };
					CAMERA_DETAILS.semanticDetails.push_back(DETAIL);
				}
				// VIEW_PROJ
				{
					ConstantSemanticDetail DETAIL = { CS_VIEW_PROJ, EF_R32G32B32A32_FLOAT, 4, MEMBER_SIZE(CAMERA, VIEW_PROJ), MEMBER_OFFSET(CAMERA, VIEW_PROJ) };
					CAMERA_DETAILS.semanticDetails.push_back(DETAIL);
				}
				// PREV_VIEW_PROJ
				{
					ConstantSemanticDetail DETAIL = { CS_PREV_VIEW_PROJ, EF_R32G32B32A32_FLOAT, 4, MEMBER_SIZE(CAMERA, PREV_VIEW_PROJ), MEMBER_OFFSET(CAMERA, PREV_VIEW_PROJ) };
					CAMERA_DETAILS.semanticDetails.push_back(DETAIL);
				}
				// PARAMETERS
				{
					ConstantSemanticDetail DETAIL = { CS_CAMERA_PARAMETERS, EF_R32G32B32A32_FLOAT, 1, MEMBER_SIZE(CAMERA, PARAMETERS), MEMBER_OFFSET(CAMERA, PARAMETERS) };
					CAMERA_DETAILS.semanticDetails.push_back(DETAIL);
				}
				CAMERA_DETAILS.bufferSize = sizeof(CAMERA);
			}

			// SHADOW
			{
				// LIGHT_VIEW
				{
					ConstantSemanticDetail DETAIL = { CS_SHADOW_VIEW, EF_R32G32B32A32_FLOAT, 4, MEMBER_SIZE(SHADOW, LIGHT_VIEW), MEMBER_OFFSET(SHADOW, LIGHT_VIEW) };
					SHADOW_DETAILS.semanticDetails.push_back(DETAIL);
				}
				// LIGHT_PROJ
				{
					ConstantSemanticDetail DETAIL = { CS_SHADOW_PROJ, EF_R32G32B32A32_FLOAT, 4, MEMBER_SIZE(SHADOW, LIGHT_PROJ), MEMBER_OFFSET(SHADOW, LIGHT_PROJ) };
					SHADOW_DETAILS.semanticDetails.push_back(DETAIL);
				}
				// CAM_NEAR_FAR
				{
					ConstantSemanticDetail DETAIL = { CS_SHADOW_CAMERA_PARAMETERS, EF_R32G32B32A32_FLOAT, 1, MEMBER_SIZE(SHADOW, PARAMETERS), MEMBER_OFFSET(SHADOW, PARAMETERS) };
					SHADOW_DETAILS.semanticDetails.push_back(DETAIL);
				}
				SHADOW_DETAILS.bufferSize = sizeof(SHADOW);
			}

			// CASCADED_SHADOW_DETAILS
			{
				// LIGHT_VIEW
				{
					ConstantSemanticDetail DETAIL = { CS_CASCADED_SHADOW_VIEW, EF_R32G32B32A32_FLOAT, 4 * 4, MEMBER_SIZE(CASCADED_SHADOW, LIGHT_VIEW), MEMBER_OFFSET(CASCADED_SHADOW, LIGHT_VIEW) };
					CASCADED_SHADOW_DETAILS.semanticDetails.push_back(DETAIL);
				}
				// LIGHT_VIEW_PROJ
				{
					ConstantSemanticDetail DETAIL = { CS_CASCADED_SHADOW_VIEW_PROJ, EF_R32G32B32A32_FLOAT, 4 * 4, MEMBER_SIZE(CASCADED_SHADOW, LIGHT_VIEW_PROJ), MEMBER_OFFSET(CASCADED_SHADOW, LIGHT_VIEW_PROJ) };
					CASCADED_SHADOW_DETAILS.semanticDetails.push_back(DETAIL);
				}
				// LIGHT_INFO
				{
					ConstantSemanticDetail DETAIL = { CS_CASCADED_SHADOW_LIGHT_INFO, EF_R32G32B32A32_FLOAT, 4, MEMBER_SIZE(CASCADED_SHADOW, LIGHT_INFO), MEMBER_OFFSET(CASCADED_SHADOW, LIGHT_INFO) };
					CASCADED_SHADOW_DETAILS.semanticDetails.push_back(DETAIL);
				}
				// FRUSTUM
				{
					ConstantSemanticDetail DETAIL = { CS_CASCADED_SHADOW_FRUSTUM, EF_R32_FLOAT, 4, MEMBER_SIZE(CASCADED_SHADOW, FRUSTUM), MEMBER_OFFSET(CASCADED_SHADOW, FRUSTUM) };
					CASCADED_SHADOW_DETAILS.semanticDetails.push_back(DETAIL);
				}
				// NUM_CASCADED
				{
					ConstantSemanticDetail DETAIL = { CS_CASCADED_SHADOW_NUM_CASCADED, EF_R32_UINT, 1, MEMBER_SIZE(CASCADED_SHADOW, NUM_CASCADED), MEMBER_OFFSET(CASCADED_SHADOW, NUM_CASCADED) };
					CASCADED_SHADOW_DETAILS.semanticDetails.push_back(DETAIL);
				}
				CASCADED_SHADOW_DETAILS.bufferSize = sizeof(CASCADED_SHADOW);
			}

			// VOXEL_DETAILS
			{
				// VIEW_PROJ
				{
					ConstantSemanticDetail DETAIL = { CS_VOXEL_VIEW_PROJ, EF_R32G32B32A32_FLOAT, 4 * 3, MEMBER_SIZE(VOXEL, VIEW_PROJ), MEMBER_OFFSET(VOXEL, VIEW_PROJ) };
					VOXEL_DETAILS.semanticDetails.push_back(DETAIL);
				}
				// VIEW_PROJ_INV
				{
					ConstantSemanticDetail DETAIL = { CS_VOXEL_VIEW_PROJ_INV, EF_R32G32B32A32_FLOAT, 4 * 3, MEMBER_SIZE(VOXEL, VIEW_PROJ_INV), MEMBER_OFFSET(VOXEL, VIEW_PROJ_INV) };
					VOXEL_DETAILS.semanticDetails.push_back(DETAIL);
				}
				// SUNLIGHT
				{
					ConstantSemanticDetail DETAIL = { CS_VOXEL_SUNLIGHT, EF_R32G32B32A32_FLOAT, 1, MEMBER_SIZE(VOXEL, SUNLIGHT), MEMBER_OFFSET(VOXEL, SUNLIGHT) };
					VOXEL_DETAILS.semanticDetails.push_back(DETAIL);
				}
				// MINPOINT_SCALE
				{
					ConstantSemanticDetail DETAIL = { CS_VOXEL_MINPOINT_SCALE, EF_R32G32B32A32_FLOAT, 1, MEMBER_SIZE(VOXEL, MINPOINT_SCALE), MEMBER_OFFSET(VOXEL, MINPOINT_SCALE) };
					VOXEL_DETAILS.semanticDetails.push_back(DETAIL);
				}
				// MISCS
				{
					ConstantSemanticDetail DETAIL = { CS_VOXEL_MISCS, EF_R32G32B32A32_UINT, 1, MEMBER_SIZE(VOXEL, MISCS), MEMBER_OFFSET(VOXEL, MISCS) };
					VOXEL_DETAILS.semanticDetails.push_back(DETAIL);
				}
				// MISCS2
				{
					ConstantSemanticDetail DETAIL = { CS_VOXEL_MISCS2, EF_R32G32B32A32_FLOAT, 1, MEMBER_SIZE(VOXEL, MISCS2), MEMBER_OFFSET(VOXEL, MISCS2) };
					VOXEL_DETAILS.semanticDetails.push_back(DETAIL);
				}
				// MISCS3
				{
					ConstantSemanticDetail DETAIL = { CS_VOXEL_MISCS3, EF_R32G32B32A32_FLOAT, 1, MEMBER_SIZE(VOXEL, MISCS3), MEMBER_OFFSET(VOXEL, MISCS3) };
					VOXEL_DETAILS.semanticDetails.push_back(DETAIL);
				}
				VOXEL_DETAILS.bufferSize = sizeof(VOXEL);
			}
			
			// GLOBAL
			{
				// SUN_LIGHT_DIR
				{
					ConstantSemanticDetail DETAIL = { CS_SUN_LIGHT_DIRECTION, EF_R32G32B32A32_FLOAT, 1, MEMBER_SIZE(GLOBAL, SUN_LIGHT_DIR), MEMBER_OFFSET(GLOBAL, SUN_LIGHT_DIR) };
					GLOBAL_DETAILS.semanticDetails.push_back(DETAIL);
				}
				GLOBAL_DETAILS.bufferSize = sizeof(GLOBAL);
			}

			CONSTANT_DETAIL_INIT = true;
		}
	}

	const ConstantBufferDetail& GetConstantBufferDetail(ConstantBufferType bufferType)
	{
		SafeInit();

		switch (bufferType)
		{
		case CBT_CAMERA:
			return CAMERA_DETAILS;
		case CBT_SHADOW:
			return SHADOW_DETAILS;
		case CBT_CASCADED_SHADOW:
			return CASCADED_SHADOW_DETAILS;
		case CBT_VOXEL:
			return VOXEL_DETAILS;
		case CBT_GLOBAL:
			return GLOBAL_DETAILS;
		default:
			assert(false && "Unknown ConstantBufferType");
			return EMPYT_DETAILS;
		}
	}
}