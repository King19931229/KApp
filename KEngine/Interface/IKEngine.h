#pragma once
#include "KRender/Interface/IKRenderCore.h"

struct IKEngine
{
	virtual ~IKEngine() {}
	virtual IKRenderCorePtr GetRenderCore() = 0;
};