#pragma once
#include "Interface/IKRenderConfig.h"
#include "Interface/IKRenderWindow.h"
#include "Interface/IKShader.h"

struct IKRenderDevice
{
	virtual ~IKRenderDevice() {}

	virtual bool Init(IKRenderWindowPtr window) = 0;
	virtual bool UnInit() = 0;

	virtual bool CreateShader(IKShaderPtr& shader) = 0;
};

EXPORT_DLL IKRenderDevicePtr CreateRenderDevice(RenderDevice platform); 