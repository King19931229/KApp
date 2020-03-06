#include "KConstantDefinition.h"

namespace KConstantDefinition
{
	static ConstantBufferDetail CAMERA_DETAILS;
	static ConstantBufferDetail SHADOW_DETAILS;
	static ConstantBufferDetail EMPYT_DETAILS;
	static bool CONSTANT_DETAIL_INIT = false;

	void SafeInit()
	{
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
		default:
			assert(false && "Unknown ConstantBufferType");
			return EMPYT_DETAILS;
		}
	}
}