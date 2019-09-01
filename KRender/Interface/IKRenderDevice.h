#pragma once
#include "KBase/Interface/IKConfig.h"
#include <memory>
#include <vector>

enum RenderDevice
{
	RD_VULKAN,
	RD_COUNT
};

struct IKRenderWindow;

struct IKRenderDevice
{
	virtual ~IKRenderDevice() {}

	virtual bool Init(IKRenderWindow* window) = 0;
	virtual bool UnInit() = 0;
};

typedef std::shared_ptr<IKRenderDevice> IKRenderDevicePtr;

EXPORT_DLL IKRenderDevicePtr CreateRenderDevice(RenderDevice platform); 