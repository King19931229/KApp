#pragma once
#include "KBase/Interface/Component/IKDebugComponent.h"
#include "KBase/Publish/KStringParser.h"
#include "KBase/Publish/KMath.h"

class KDebugComponent : public IKDebugComponent, public KReflectionObjectBase
{
	RTTR_ENABLE(IKDebugComponent, KReflectionObjectBase)
	RTTR_REGISTRATION_FRIEND
protected:
public:
	KDebugComponent();
	virtual ~KDebugComponent();

	bool Save(IKXMLElementPtr element) override
	{
		return true;
	}

	bool Load(IKXMLElementPtr element) override
	{
		return true;
	}
};