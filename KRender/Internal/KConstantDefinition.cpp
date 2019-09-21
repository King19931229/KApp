#include "KConstantDefinition.h"

#define MEMBER_OFFSET(structure, member) ((int)&((structure*)0)->member)
#define MEMBER_SIZE(structure, member) sizeof(((structure*)0)->member)

namespace KConstantDefinition
{
	static ConstantBufferDetail TRANSFORM_DETAILS;
	static ConstantBufferDetail EMPYT_DETAILS;
	static bool CONSTANT_DETAIL_INIT = false;

	void SafeInit()
	{
		if(!CONSTANT_DETAIL_INIT)
		{
			// TRANSFORM
			{
				// MODEL
				{
					ConstantSemanticDetail DETAIL = { CS_MODEL, EF_R32G32B32A32_FLOAT, 4, MEMBER_SIZE(TRANSFORM, MODEL), MEMBER_OFFSET(TRANSFORM, MODEL) };
					TRANSFORM_DETAILS.semanticDetails.push_back(DETAIL);
				}
				// VIEW
				{
					ConstantSemanticDetail DETAIL = { CS_VIEW, EF_R32G32B32A32_FLOAT, 4, MEMBER_SIZE(TRANSFORM, VIEW), MEMBER_OFFSET(TRANSFORM, VIEW) };
					TRANSFORM_DETAILS.semanticDetails.push_back(DETAIL);
				}
				// PROJ
				{
					ConstantSemanticDetail DETAIL = { CS_PROJ, EF_R32G32B32A32_FLOAT, 4, MEMBER_SIZE(TRANSFORM, PROJ), MEMBER_OFFSET(TRANSFORM, PROJ) };
					TRANSFORM_DETAILS.semanticDetails.push_back(DETAIL);
				}
				TRANSFORM_DETAILS.bufferSize = sizeof(TRANSFORM);
			}
			CONSTANT_DETAIL_INIT = true;
		}
	}

	const ConstantBufferDetail& GetConstantBufferDetail(ConstantBufferType bufferType)
	{
		SafeInit();
		switch (bufferType)
		{
		case CBT_TRANSFORM:
			return TRANSFORM_DETAILS;
		default:
			return EMPYT_DETAILS;
		}
	}
}