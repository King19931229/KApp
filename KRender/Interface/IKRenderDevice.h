#pragma once
#include "KBase/Interface/IKConfig.h"
#include <memory>
#include <vector>

enum RenderDevice
{
	RD_VULKAN,
	RD_COUNT
};

struct IKRenderDevice
{
	struct ExtensionProperties
	{
		std::string property;
		unsigned int specVersion;
	};
	typedef std::vector<ExtensionProperties> DeviceExtensions;

	virtual ~IKRenderDevice() {}

	virtual bool Init() = 0;
	virtual bool UnInit() = 0;

	virtual bool QueryExtensions(DeviceExtensions& exts) = 0;
};

typedef std::shared_ptr<IKRenderDevice> IKRenderDevicePtr;

EXPORT_DLL IKRenderDevicePtr CreateRenderDevice(RenderDevice platform); 