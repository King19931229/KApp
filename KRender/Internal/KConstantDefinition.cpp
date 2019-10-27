#include "KConstantDefinition.h"

namespace KConstantDefinition
{
	static ConstantBufferDetail OBJECT_DETAILS;
	static ConstantBufferDetail CAMERA_DETAILS;
	static ConstantBufferDetail EMPYT_DETAILS;
	static bool CONSTANT_DETAIL_INIT = false;

	void SafeInit()
	{
		if(!CONSTANT_DETAIL_INIT)
		{
			// OBJECT
			{
				// MODEL
				{
					ConstantSemanticDetail DETAIL = { CS_MODEL, EF_R32G32B32A32_FLOAT, 4, MEMBER_SIZE(OBJECT, MODEL), MEMBER_OFFSET(OBJECT, MODEL) };
					OBJECT_DETAILS.semanticDetails.push_back(DETAIL);
				}
				OBJECT_DETAILS.bufferSize = sizeof(OBJECT);
			}

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
			CONSTANT_DETAIL_INIT = true;
		}
	}

	const ConstantBufferDetail& GetConstantBufferDetail(ConstantBufferType bufferType)
	{
		SafeInit();
		switch (bufferType)
		{
		case CBT_OBJECT:
			return OBJECT_DETAILS;
		case CBT_CAMERA:
			return CAMERA_DETAILS;
		default:
			assert(false && "Unknown ConstantBufferType");
			return EMPYT_DETAILS;
		}
	}
}