#pragma once
#include "IKComponentBase.h"
#include "KBase/Publish/KDebugUtility.h"
#include "KBase/Publish/KAABBBox.h"

struct IKDebugComponent : public IKComponentBase
{
	RTTR_ENABLE(IKComponentBase)
	RTTR_REGISTRATION_FRIEND
public:
	IKDebugComponent()
		: IKComponentBase(CT_DEBUG)
	{
	}
	virtual ~IKDebugComponent() {}

	virtual bool AddDebugPart(const KDebugUtilityInfo& info, const glm::vec4& color) = 0;
	virtual void DestroyAllDebugParts() = 0;

	virtual bool GetBound(KAABBBox& bound) const = 0;
};