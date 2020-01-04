#pragma once
#include "KRender/Interface/IKRenderConfig.h"

struct IKRenderCore
{
	virtual ~IKRenderCore() {}

	virtual bool Init(RenderDevice device, size_t windowWidth, size_t windowHeight) = 0;
	virtual bool UnInit() = 0;
	virtual bool Loop() = 0;

	virtual IKRenderWindow* GetRenderWindow() = 0;
};
typedef std::unique_ptr<IKRenderCore> IKRenderCorePtr;

EXPORT_DLL IKRenderCorePtr CreateRenderCore(); 