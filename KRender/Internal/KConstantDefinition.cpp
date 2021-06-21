#include "KConstantDefinition.h"

namespace KConstantDefinition
{
	static ConstantBufferDetail CAMERA_DETAILS;
	static ConstantBufferDetail SHADOW_DETAILS;
	static ConstantBufferDetail CASCADED_SHADOW_DETAILS;
	static ConstantBufferDetail GLOBAL_DETAILS;
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
					ConstantSemanticDetail DETAIL = { CS_SHADOW_NEAR_FAR, EF_R32G32_FLOAT, 1, MEMBER_SIZE(SHADOW, CAM_NEAR_FAR), MEMBER_OFFSET(SHADOW, CAM_NEAR_FAR) };
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
		case CBT_GLOBAL:
			return GLOBAL_DETAILS;
		default:
			assert(false && "Unknown ConstantBufferType");
			return EMPYT_DETAILS;
		}
	}
}