#pragma once
#include "KBase/Interface/IKConfig.h"
#include <memory>

enum RenderDevice
{
	RD_VULKAN,
	RD_COUNT
};

struct IKRenderWindow;
typedef std::shared_ptr<IKRenderWindow> IKRenderWindowPtr;

struct IKRenderDevice;
typedef std::shared_ptr<IKRenderDevice> IKRenderDevicePtr;

struct IKShader;
typedef std::shared_ptr<IKShader> IKShaderPtr;