#pragma once
#include "KBase/Interface/IKConfig.h"
#include "Interface/IKRenderDevice.h"

#include <memory>

struct IKRenderWindow
{
	virtual ~IKRenderWindow() {}

	virtual bool Init(size_t top, size_t left, size_t width, size_t height) = 0;
	virtual bool UnInit() = 0;

	virtual bool Loop() = 0;

	virtual bool GetPosition(size_t &top, size_t &left) = 0;
	virtual bool SetPosition(size_t top, size_t left) = 0;

	virtual bool GetSize(size_t &width, size_t &height) = 0;
	virtual bool SetSize(size_t width, size_t height) = 0;
};

typedef std::shared_ptr<IKRenderWindow> IKRenderWindowPtr;

EXPORT_DLL IKRenderWindowPtr CreateRenderWindow(RenderDevice platform);